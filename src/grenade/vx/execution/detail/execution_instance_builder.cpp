#include "grenade/vx/execution/detail/execution_instance_builder.h"

#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/execution/detail/execution_instance_config_visitor.h"
#include "grenade/vx/execution/detail/generator/madc.h"
#include "grenade/vx/execution/detail/generator/ppu.h"
#include "grenade/vx/execution/detail/generator/timed_spike_to_chip_sequence.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/detail/extmem.h"
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "grenade/vx/signal_flow/types.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/block.h"
#include "haldls/vx/v3/fpga.h"
#include "haldls/vx/v3/padi.h"
#include "hate/math.h"
#include "hate/timer.h"
#include "hate/type_traits.h"
#include "hate/variant.h"
#include "lola/vx/ppu.h"
#include "stadls/vx/constants.h"
#include "stadls/vx/playback_generator.h"
#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <boost/range/combine.hpp>
#include <boost/sort/spinsort/spinsort.hpp>
#include <boost/type_index.hpp>
#include <log4cxx/logger.h>
#include <tbb/parallel_for_each.h>

namespace grenade::vx::execution::detail {

namespace {

template <typename T>
std::string name()
{
	auto const full = boost::typeindex::type_id<T>().pretty_name();
	return full.substr(full.rfind(':') + 1);
}

} // namespace

ExecutionInstanceBuilder::ExecutionInstanceBuilder(
    signal_flow::Graph const& graph,
    common::ExecutionInstanceID const& execution_instance,
    signal_flow::IODataMap const& input_list,
    signal_flow::IODataMap const& data_output,
    std::optional<lola::vx::v3::PPUElfFile::symbols_type> const& ppu_symbols,
    signal_flow::ExecutionInstancePlaybackHooks& playback_hooks) :
    m_graph(graph),
    m_execution_instance(execution_instance),
    m_input_list(input_list),
    m_data_output(data_output),
    m_ppu_symbols(ppu_symbols),
    m_playback_hooks(playback_hooks),
    m_post_vertices(),
    m_local_data(),
    m_local_data_output(),
    m_madc_readout_vertex()
{
	// check that input list provides all requested input for local graph
	if (!has_complete_input_list()) {
		throw std::runtime_error("Graph requests unprovided input.");
	}

	m_ticket_requests.fill(false);

	m_postprocessing = false;
	size_t const batch_size = m_input_list.batch_size();
	m_batch_entries.resize(batch_size);
}

bool ExecutionInstanceBuilder::has_complete_input_list() const
{
	if (m_input_list.empty()) {
		return true;
	}
	if (!m_input_list.valid()) {
		return false;
	}
	auto const batch_size = m_input_list.batch_size();
	auto const execution_instance_vertex =
	    m_graph.get_execution_instance_map().right.at(m_execution_instance);
	auto const vertices = boost::make_iterator_range(
	    m_graph.get_vertex_descriptor_map().right.equal_range(execution_instance_vertex));
	return std::none_of(vertices.begin(), vertices.end(), [&](auto const& p) {
		auto const vertex = p.second;
		if (std::holds_alternative<signal_flow::vertex::ExternalInput>(
		        m_graph.get_vertex_property(vertex))) {
			if (m_input_list.data.find(vertex) == m_input_list.data.end()) {
				return true;
			}
			if (batch_size == 0) {
				return false;
			}
			auto const& input_vertex =
			    std::get<signal_flow::vertex::ExternalInput>(m_graph.get_vertex_property(vertex));
			return !signal_flow::IODataMap::is_match(
			    m_input_list.data.find(vertex)->second, input_vertex.output());
		}
		return false;
	});
}

bool ExecutionInstanceBuilder::inputs_available(
    signal_flow::Graph::vertex_descriptor const descriptor) const
{
	auto const edges = boost::make_iterator_range(boost::in_edges(descriptor, m_graph.get_graph()));
	return std::all_of(edges.begin(), edges.end(), [&](auto const& edge) -> bool {
		auto const in_vertex = boost::source(edge, m_graph.get_graph());
		return std::find(m_post_vertices.begin(), m_post_vertices.end(), in_vertex) ==
		       m_post_vertices.end();
	});
}

template <typename T>
void ExecutionInstanceBuilder::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */, T const& /* data */)
{
	// Specialize for types which are not empty
}

template <>
void ExecutionInstanceBuilder::process(
    signal_flow::Graph::vertex_descriptor const /*vertex*/,
    signal_flow::vertex::NeuronView const& data)
{
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	size_t i = 0;
	auto const& configs = data.get_configs();
	for (auto const column : data.get_columns()) {
		auto const neuron_reset = AtomicNeuronOnDLS(column, data.get_row()).toNeuronResetOnDLS();
		m_neuron_resets.enable_resets[neuron_reset] = configs.at(i).enable_reset;
		i++;
	}
}

template <>
void ExecutionInstanceBuilder::process(
    signal_flow::Graph::vertex_descriptor const vertex, signal_flow::vertex::CrossbarL2Input const&)
{
	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
	auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
	auto const in_vertex = boost::source(*(in_edges.first), m_graph.get_graph());

	m_local_data.data[vertex] = m_local_data.data.at(in_vertex);
	m_event_input_vertex = vertex;
}

template <>
void ExecutionInstanceBuilder::process(
    signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::CrossbarL2Output const&)
{
	if (!m_postprocessing) {
		if (m_event_output_vertex) {
			throw std::logic_error("Only one event output vertex allowed.");
		}
		m_event_output_vertex = vertex;
		m_post_vertices.push_back(vertex);
	} else {
		auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceBuilder");

		std::vector<stadls::vx::PlaybackProgram::spikes_type> spikes(m_batch_entries.size());
		for (auto const& program : m_chunked_program) {
			auto local_spikes = program.get_spikes();

			LOG4CXX_INFO(logger, "process(): " << local_spikes.size() << " spikes");

			filter_events(spikes, std::move(local_spikes));
		}
		std::vector<signal_flow::TimedSpikeFromChipSequence> transformed_spikes(spikes.size());
		for (size_t i = 0; i < spikes.size(); ++i) {
			auto& local_transformed_spikes = transformed_spikes.at(i);
			auto const& local_spikes = spikes.at(i);
			local_transformed_spikes.reserve(local_spikes.size());
			for (auto const& local_spike : local_spikes) {
				local_transformed_spikes.push_back(signal_flow::TimedSpikeFromChip(
				    common::Time(local_spike.chip_time.value()), local_spike.label));
			}
		}
		m_local_data.data[vertex] = std::move(transformed_spikes);
	}
}

namespace {

template <typename T>
std::vector<common::TimedDataSequence<std::vector<T>>> apply_restriction(
    std::vector<common::TimedDataSequence<std::vector<T>>> const& value,
    signal_flow::PortRestriction const& restriction)
{
	std::vector<common::TimedDataSequence<std::vector<T>>> ret(value.size());
	for (size_t b = 0; b < ret.size(); ++b) {
		auto& local_ret = ret.at(b);
		auto const& local_value = value.at(b);
		local_ret.resize(local_value.size());
		for (size_t bb = 0; bb < local_value.size(); ++bb) {
			local_ret.at(bb).data.insert(
			    local_ret.at(bb).data.end(), local_value.at(bb).data.begin() + restriction.min(),
			    local_value.at(bb).data.begin() + restriction.max() + 1);
			local_ret.at(bb).time = local_value.at(bb).time;
		}
	}
	return ret;
}

} // namespace

template <>
void ExecutionInstanceBuilder::process(
    signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::DataInput const& /* data */)
{
	using namespace lola::vx::v3;
	using namespace haldls::vx::v3;
	using namespace halco::hicann_dls::vx::v3;
	using namespace halco::common;

	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
	auto const edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
	auto const in_vertex = boost::source(edge, m_graph.get_graph());
	auto const& input_values =
	    ((std::holds_alternative<signal_flow::vertex::ExternalInput>(
	         m_graph.get_vertex_property(in_vertex)))
	         ? m_input_list.data.at(in_vertex)
	         : m_data_output.data.at(in_vertex));

	auto const maybe_apply_restriction = [&](auto const& d) -> signal_flow::IODataMap::Entry {
		typedef std::remove_cvref_t<decltype(d)> Data;
		typedef hate::type_list<
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>
		    MatrixData;
		if constexpr (hate::is_in_type_list<Data, MatrixData>::value) {
			auto const port_restriction = m_graph.get_edge_property_map().at(edge);
			if (port_restriction) {
				return apply_restriction(d, *port_restriction);
			}
		}
		return d;
	};
	m_local_data.data[vertex] = std::visit(maybe_apply_restriction, input_values);
}

namespace {

template <typename T>
void resize_rectangular(std::vector<std::vector<T>>& data, size_t size_outer, size_t size_inner)
{
	data.resize(size_outer);
	for (auto& inner : data) {
		inner.resize(size_inner);
	}
}

} // namespace

template <>
void ExecutionInstanceBuilder::process(
    signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::CADCMembraneReadoutView const& data)
{
	// check mode and save
	if (m_cadc_readout_mode) {
		if (*m_cadc_readout_mode != data.get_mode()) {
			throw std::runtime_error("Heterogenous CADC readout modes not supported.");
		}
	} else {
		m_cadc_readout_mode = data.get_mode();
	}

	// get source NeuronView
	auto const synram = data.get_synram();
	auto const hemisphere = synram.toHemisphereOnDLS();
	auto const& columns = data.get_columns();

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace lola::vx::v3;
	using namespace haldls::vx::v3;
	if (!m_postprocessing) { // pre-hw-run processing
		m_ticket_requests[hemisphere] = true;
		// results need hardware execution
		m_post_vertices.push_back(vertex);
	} else { // post-hw-run processing
		// extract signal_flow::Int8 values
		std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>> sample_batches(
		    m_batch_entries.size());
		assert(m_cadc_readout_mode);
		if (*m_cadc_readout_mode == signal_flow::vertex::CADCMembraneReadoutView::Mode::hagen) {
			for (auto& e : sample_batches) {
				e.resize(1);
				// TODO: Think about what to do with timing information
				e.at(0).data.resize(data.output().size);
			}
			for (size_t batch_index = 0; batch_index < m_batch_entries.size(); ++batch_index) {
				assert(m_batch_entries.at(batch_index).m_ppu_result[synram.toPPUOnDLS()]);
				auto const block = dynamic_cast<PPUMemoryBlock const&>(
				    m_batch_entries.at(batch_index).m_ppu_result[synram.toPPUOnDLS()]->get());
				auto const values = from_vector_unit_row(block);
				auto& samples = sample_batches.at(batch_index).at(0).data;

				// get samples via neuron mapping from incoming NeuronView
				size_t i = 0;
				for (auto const& column_collection : columns) {
					for (auto const& column : column_collection) {
						samples.at(i) = signal_flow::Int8(values[column.toNeuronColumnOnDLS()]);
						i++;
					}
				}
			}
		} else {
			for (size_t batch_index = 0; batch_index < m_batch_entries.size(); ++batch_index) {
				assert(m_batch_entries.at(batch_index).m_extmem_result[synram.toPPUOnDLS()]);
				auto const local_block = dynamic_cast<ExternalPPUMemoryBlock const&>(
				    m_batch_entries.at(batch_index).m_extmem_result[synram.toPPUOnDLS()]->get());

				uint32_t const local_block_size_expectation =
				    (local_block.size() - ppu_vector_alignment /* num samples */) /
				    (ppu_vector_alignment /* sample time stamp */ +
				     2 * ppu_vector_alignment /* sample values */);
				// get number of samples
				uint32_t num_samples = 0;
				for (size_t i = 0; i < 4; ++i) {
					num_samples |= static_cast<uint32_t>(local_block.at(i).get_value().value())
					               << (3 - i) * CHAR_BIT;
				}
				if (num_samples > local_block_size_expectation) {
					auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceBuilder");
					LOG4CXX_WARN(
					    logger, "Less CADC samples read-out (" << local_block_size_expectation
					                                           << ") than recorded (" << num_samples
					                                           << ") during execution.");
				}
				num_samples = std::min(num_samples, local_block_size_expectation);

				size_t offset = ppu_vector_alignment;
				// get samples
				auto& samples = sample_batches.at(batch_index);
				samples.resize(num_samples);
				for (size_t i = 0; i < num_samples; ++i) {
					auto& local_samples = samples.at(i);
					uint64_t time = 0;
					for (size_t j = 0; j < 8; ++j) {
						time |=
						    static_cast<uint64_t>(local_block.at(offset + j).get_value().value())
						    << (7 - j) * CHAR_BIT;
					}
					local_samples.time =
					    common::Time(time / 2); // FPGA clock 125MHz vs. PPU clock 250MHz
					offset += ppu_vector_alignment;
					auto const get_index = [](auto const& column) {
						size_t const j = column / 2;
						size_t const index = (ppu_vector_alignment - 1) -
						                     ((j / 4) * 4 + (3 - j % 4)) +
						                     (column % 2) * ppu_vector_alignment;
						return index;
					};
					local_samples.data.resize(data.output().size);
					for (size_t j = 0; auto const& column_collection : columns) {
						for (auto const& column : column_collection) {
							local_samples.data.at(j) =
							    signal_flow::Int8(local_block.at(offset + get_index(column.value()))
							                          .get_value()
							                          .value());
							j++;
						}
					}
					offset += 256;
				}
				// Since there's no time synchronisation between PPUs and ChipTime, we assume the
				// first received sample happens at time 0 and calculate the time of later samples
				// from that reference point via the given PPU-local counter value.
				if (!samples.empty()) {
					auto const time_0 = samples.at(0).time;
					for (auto& entry : samples) {
						entry.time -= time_0;
					}
				}
			}
		}
		m_local_data.data[vertex] = sample_batches;
	}
}

template <>
void ExecutionInstanceBuilder::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::ExternalInput const& /* data */)
{}

template <>
void ExecutionInstanceBuilder::process(
    [[maybe_unused]] signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::DataOutput const& data)
{
	// get in edge
	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
	auto const in_edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
	auto const& local_data = m_local_data.data.at(boost::source(in_edge, m_graph.get_graph()));
	// maybe apply port restriction
	auto const maybe_apply_restriction = [&](auto const& d) -> signal_flow::IODataMap::Entry {
		typedef std::remove_cvref_t<decltype(d)> Data;
		typedef hate::type_list<
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>
		    MatrixData;
		if constexpr (hate::is_in_type_list<Data, MatrixData>::value) {
			auto const port_restriction = m_graph.get_edge_property_map().at(in_edge);
			if (port_restriction) {
				return apply_restriction(d, *port_restriction);
			}
		}
		return d;
	};
	auto const maybe_restricted_local_data = std::visit(maybe_apply_restriction, local_data);
	// check size match only for first because we know that the data map is valid
	assert(signal_flow::IODataMap::is_match(maybe_restricted_local_data, data.output()));
	m_local_data_output.data[vertex] = maybe_restricted_local_data;
}

template <typename T>
void ExecutionInstanceBuilder::filter_events(
    std::vector<std::vector<T>>& filtered_data, std::vector<T>&& data) const
{
	// early return if no events are recorded
	if (data.empty()) {
		return;
	}
	// sort events by chip time
	boost::sort::spinsort(data.begin(), data.end(), [](auto const& a, auto const& b) {
		return a.chip_time < b.chip_time;
	});
	// iterate over batch entries and extract associated events
	auto begin = data.begin();
	for (size_t i = 0; auto const& e : m_batch_entries) {
		// Extract all events in-between the interval from the event begin FPGATime value to the
		// event end FPGATime value comparing with the ChipTime value of the events. Comparing the
		// FPGATime of the interval bounds with the ChipTime of the events leads to a small drift of
		// the interval towards the past, i.e. the recording starts and stops a one-way
		// highspeed-link latency too early. We define the to be recorded interval in this way.
		// In addition, valid relative times are restricted to be within [0, runtime).

		// find begin of interval
		assert(e.m_ticket_events_begin);
		auto const interval_begin_time = e.m_ticket_events_begin->get_fpga_time().value();
		begin = std::find_if(begin, data.end(), [&](auto const& event) {
			return event.chip_time.value() >= interval_begin_time;
		});
		// if no spikes are recorded for this data return
		if (begin == data.end()) {
			return;
		}
		// find end of interval
		assert(e.m_ticket_events_end);
		auto const interval_end_time = e.m_ticket_events_end->get_fpga_time().value();
		uintmax_t end_time = 0;
		if (!m_input_list.runtime.empty() &&
		    m_input_list.runtime.at(i).contains(m_execution_instance)) {
			auto const absolute_end_chip_time =
			    m_input_list.runtime.at(i).at(m_execution_instance).value() + interval_begin_time;
			end_time = std::min(interval_end_time, absolute_end_chip_time);
		} else {
			end_time = interval_end_time;
		}
		typename std::vector<T>::iterator end;
		// find in reversed order if this entry is the last batch entry, leads to O(1) in this case.
		if (i == m_batch_entries.size() - 1) {
			end = std::find_if_not(
			          data.rbegin(), typename std::vector<T>::reverse_iterator(begin),
			          [&](auto const& event) { return event.chip_time.value() >= end_time; })
			          .base();
		} else {
			end = std::find_if(begin, data.end(), [&](auto const& event) {
				return event.chip_time.value() >= end_time;
			});
		}
		// copy interval content
		std::vector<T> data_batch(begin, end);
		// subtract the interval begin time to get relative times
		for (auto& event : data_batch) {
			event.chip_time = haldls::vx::v3::ChipTime(event.chip_time - interval_begin_time);
		}
		if (!data_batch.empty()) {
			filtered_data.at(i) = std::move(data_batch);
		}
		begin = end;
		i++;
	}
}

template <>
void ExecutionInstanceBuilder::process(
    signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::MADCReadoutView const& data)
{
	if (!m_postprocessing) {
		if (m_madc_readout_vertex) {
			throw std::logic_error("Only one MADC readout vertex allowed.");
		}
		m_madc_readout_vertex = vertex;
		m_post_vertices.push_back(vertex);
	} else {
		auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceBuilder");

		std::vector<stadls::vx::PlaybackProgram::madc_samples_type> madc_samples(
		    m_batch_entries.size());
		for (auto const& program : m_chunked_program) {
			auto local_madc_samples = program.get_madc_samples();

			LOG4CXX_INFO(logger, "process(): " << local_madc_samples.size() << " MADC samples");

			filter_events(madc_samples, std::move(local_madc_samples));
		}
		std::vector<signal_flow::TimedMADCSampleFromChipSequence> transformed_madc_samples(
		    madc_samples.size());
		for (size_t i = 0; i < madc_samples.size(); ++i) {
			auto& local_transformed_madc_samples = transformed_madc_samples.at(i);
			auto const& local_madc_samples = madc_samples.at(i);
			local_transformed_madc_samples.reserve(local_madc_samples.size());
			for (auto const& local_madc_sample : local_madc_samples) {
				// Inverting channel fro 2ch recording due to Issue #3998
				local_transformed_madc_samples.push_back(signal_flow::TimedMADCSampleFromChip(
				    common::Time(local_madc_sample.chip_time.value()),
				    signal_flow::MADCSampleFromChip(
				        local_madc_sample.value, static_cast<bool>(data.get_second_source())
				                                     ? signal_flow::MADCSampleFromChip::Channel(
				                                           1 - local_madc_sample.channel)
				                                     : local_madc_sample.channel)));
			}
		}
		m_local_data.data[vertex] = std::move(transformed_madc_samples);
	}
}

template <>
void ExecutionInstanceBuilder::process(
    signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::Transformation const& data)
{
	// fill input value
	auto const inputs = data.inputs();
	std::vector<signal_flow::vertex::Transformation::Function::Value> value_input;
	auto edge_it = boost::in_edges(vertex, m_graph.get_graph()).first;
	for (auto const& port : inputs) {
		if (m_graph.get_edge_property_map().at(*edge_it)) {
			throw std::logic_error("Edge with port restriction unsupported.");
		}
		auto const in_vertex = boost::source(*edge_it, m_graph.get_graph());
		auto const& input_values =
		    ((std::holds_alternative<signal_flow::vertex::ExternalInput>(
		         m_graph.get_vertex_property(in_vertex)))
		         ? m_input_list.data.at(in_vertex)
		         : m_local_data.data.at(in_vertex));
		if (!signal_flow::IODataMap::is_match(input_values, port)) {
			throw std::runtime_error("Data size does not match expectation.");
		}
		value_input.push_back(input_values);
		edge_it++;
	}
	// execute transformation
	auto const value_output = data.apply(value_input);
	// process output value
	if (!signal_flow::IODataMap::is_match(value_output, data.output())) {
		throw std::runtime_error("Data size does not match expectation.");
	}
	m_local_data.data[vertex] = value_output;
}

template <>
void ExecutionInstanceBuilder::process(
    signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::PlasticityRule const& data)
{
	m_has_plasticity_rule = true;
	if (!m_postprocessing) {
		if (!data.get_recording()) {
			return;
		}
		if (data.get_recorded_scratchpad_memory_size() % sizeof(uint32_t)) {
			throw std::runtime_error(
			    "Recorded scratchpad memory size needs to be a multiple of four.");
		}
		for (auto& batch_entry : m_batch_entries) {
			batch_entry.m_plasticity_rule_recorded_scratchpad_memory[vertex].fill(std::nullopt);
		}
		m_post_vertices.push_back(vertex);
	} else {
		if (!data.get_recording()) {
			return;
		}
		std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>> values(
		    m_input_list.batch_size());
		for (size_t i = 0; i < values.size(); ++i) {
			auto& local_values = values.at(i);
			if (std::holds_alternative<signal_flow::vertex::PlasticityRule::RawRecording>(
			        *data.get_recording())) {
				// TODO: Think about what shall happen with timing info
				local_values.resize(1);
				local_values.at(0).data.resize(data.output().size);
				for (auto const ppu :
				     halco::common::iter_all<halco::hicann_dls::vx::v3::PPUOnDLS>()) {
					auto const& local_ticket =
					    m_batch_entries.at(i).m_plasticity_rule_recorded_scratchpad_memory.at(
					        vertex)[ppu];
					assert(local_ticket);
					auto const bytes = dynamic_cast<lola::vx::v3::ExternalPPUMemoryBlock const&>(
					                       local_ticket->get())
					                       .get_bytes();
					if (bytes.size() != data.get_recorded_scratchpad_memory_size()) {
						throw std::logic_error(
						    "Recording scratchpad memory size (" + std::to_string(bytes.size()) +
						    ") does not match expectation(" +
						    std::to_string(data.get_recorded_scratchpad_memory_size()) + ").");
					}
					size_t const offset = ppu ? local_values.at(0).data.size() / 2 : 0;
					for (size_t j = 0; j < bytes.size(); ++j) {
						local_values.at(0).data.at(offset + j) =
						    signal_flow::Int8(bytes.at(j).get_value().value());
					}
				}
			} else if (std::holds_alternative<signal_flow::vertex::PlasticityRule::TimedRecording>(
			               *data.get_recording())) {
				local_values.resize(data.get_timer().num_periods);
				for (auto& e : local_values) {
					e.data.resize(data.output().size);
				}
				for (auto const ppu :
				     halco::common::iter_all<halco::hicann_dls::vx::v3::PPUOnDLS>()) {
					auto const& local_ticket =
					    m_batch_entries.at(i).m_plasticity_rule_recorded_scratchpad_memory.at(
					        vertex)[ppu];
					assert(local_ticket);
					auto const bytes = dynamic_cast<lola::vx::v3::ExternalPPUMemoryBlock const&>(
					                       local_ticket->get())
					                       .get_bytes();
					if (bytes.size() !=
					    data.get_recorded_scratchpad_memory_size() * data.get_timer().num_periods) {
						throw std::logic_error(
						    "Recording scratchpad memory size (" + std::to_string(bytes.size()) +
						    ") does not match expectation(" +
						    std::to_string(
						        data.get_recorded_scratchpad_memory_size() *
						        data.get_timer().num_periods) +
						    ").");
					}
					size_t const offset = ppu ? local_values.at(0).data.size() / 2 : 0;
					for (size_t period = 0; period < data.get_timer().num_periods; ++period) {
						uint64_t time = 0;
						size_t period_offset = period * data.get_recorded_scratchpad_memory_size();
						time |= static_cast<uint64_t>(
						            (bytes.at(period_offset + 0).get_value().value()) & 0xff)
						        << 56;
						time |= static_cast<uint64_t>(
						            (bytes.at(period_offset + 1).get_value().value()) & 0xff)
						        << 48;
						time |= static_cast<uint64_t>(
						            (bytes.at(period_offset + 2).get_value().value()) & 0xff)
						        << 40;
						time |= static_cast<uint64_t>(
						            (bytes.at(period_offset + 3).get_value().value()) & 0xff)
						        << 32;
						time |= static_cast<uint64_t>(
						            (bytes.at(period_offset + 4).get_value().value()) & 0xff)
						        << 24;
						time |= static_cast<uint64_t>(
						            (bytes.at(period_offset + 5).get_value().value()) & 0xff)
						        << 16;
						time |= static_cast<uint64_t>(
						            (bytes.at(period_offset + 6).get_value().value()) & 0xff)
						        << 8;
						time |= static_cast<uint64_t>(
						            (bytes.at(period_offset + 7).get_value().value()) & 0xff)
						        << 0;
						// TODO: decide what to do with the other time of the other PPU (top).
						local_values.at(period).time = common::Time(time / 2);
						for (size_t j = 0; j < local_values.at(period).data.size() / 2 /** PPUs */;
						     ++j) {
							local_values.at(period).data.at(offset + j) = signal_flow::Int8(
							    bytes
							        .at(period_offset +
							            data.get_recorded_memory_data_interval().first + j)
							        .get_value()
							        .value());
						}
					}
				}
			} else {
				throw std::logic_error("Recording type not supported.");
			}
		}
		m_local_data.data[vertex] = values;
	}
}

ExecutionInstanceBuilder::Usages ExecutionInstanceBuilder::pre_process()
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceBuilder");
	auto const execution_instance_vertex =
	    m_graph.get_execution_instance_map().right.at(m_execution_instance);
	// Sequential preprocessing because vertices might depend on each other.
	for (auto const p : boost::make_iterator_range(
	         m_graph.get_vertex_descriptor_map().right.equal_range(execution_instance_vertex))) {
		auto const vertex = p.second;
		if (inputs_available(vertex)) {
			std::visit(
			    [&](auto const& value) {
				    hate::Timer timer;
				    process(vertex, value);
				    LOG4CXX_TRACE(
				        logger, "process(): Preprocessed "
				                    << name<hate::remove_all_qualifiers_t<decltype(value)>>()
				                    << " in " << timer.print() << ".");
			    },
			    m_graph.get_vertex_property(vertex));
		} else {
			m_post_vertices.push_back(vertex);
		}
	}
	return ExecutionInstanceBuilder::Usages{.madc_recording = m_madc_readout_vertex.has_value(), .event_recording = m_event_output_vertex.has_value() || m_madc_readout_vertex.has_value()};
}

signal_flow::IODataMap ExecutionInstanceBuilder::post_process(
    std::vector<stadls::vx::v3::PlaybackProgram> const& realtime)
{
	m_chunked_program = realtime;

	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceBuilder");

	auto const has_ppu_usage = [](auto const& be) { return !be.ppu_symbols.empty(); };
	if (std::any_of(m_batch_entries.begin(), m_batch_entries.end(), has_ppu_usage)) {
		assert(std::all_of(m_batch_entries.begin(), m_batch_entries.end(), has_ppu_usage));
		m_local_data_output.read_ppu_symbols.resize(m_batch_entries.size());
	}

	std::chrono::nanoseconds total_realtime_duration{0};
	for (size_t batch_index = 0; batch_index < m_batch_entries.size(); ++batch_index) {
		auto const& batch_entry = m_batch_entries.at(batch_index);
		for (auto const ppu : halco::common::iter_all<halco::hicann_dls::vx::v3::PPUOnDLS>()) {
			auto const& scheduler_finished = batch_entry.m_ppu_scheduler_finished[ppu];
			if (scheduler_finished) {
				auto const value = ppu::detail::Status(
				    dynamic_cast<haldls::vx::v3::PPUMemoryBlock const&>(scheduler_finished->get())
				        .at(0)
				        .get_value()
				        .value());
				if (value != ppu::detail::Status::idle) {
					LOG4CXX_ERROR(
					    logger, "On-PPU scheduler didn't finish operation (" << ppu << ").");
				}
			}
			auto const& scheduler_event_drop_count =
			    batch_entry.m_ppu_scheduler_event_drop_count[ppu];
			if (scheduler_event_drop_count) {
				auto const value = dynamic_cast<haldls::vx::v3::PPUMemoryBlock const&>(
				                       scheduler_event_drop_count->get())
				                       .at(0)
				                       .get_value()
				                       .value();
				if (value != 0) {
					LOG4CXX_ERROR(
					    logger, "On-PPU scheduler could not execute all tasks ("
					                << ppu << ", dropped: " << value << ").");
				}
			}
			for (auto const& timer_event_drop_counts : batch_entry.m_ppu_timer_event_drop_count) {
				if (timer_event_drop_counts[ppu]) {
					auto const value = dynamic_cast<haldls::vx::v3::PPUMemoryBlock const&>(
					                       timer_event_drop_counts[ppu]->get())
					                       .at(0)
					                       .get_value()
					                       .value();
					if (value != 0) {
						LOG4CXX_ERROR(
						    logger, "On-PPU timer could not insert all requested tasks into "
						            "execution queue ("
						                << ppu << ", skipped: " << value << ").");
					}
				}
			}
			if (batch_entry.m_ppu_mailbox[ppu]) {
				auto mailbox = dynamic_cast<haldls::vx::v3::PPUMemoryBlock const&>(
				    batch_entry.m_ppu_mailbox[ppu]->get());
				LOG4CXX_DEBUG(
				    logger, "PPU(" << ppu.value() << ") mailbox:\n"
				                   << mailbox.to_string());
			}
		}
		for (auto const& [name, ticket] : batch_entry.ppu_symbols) {
			std::visit(
			    hate::overloaded{
			        [&](std::map<
			            halco::hicann_dls::vx::v3::HemisphereOnDLS,
			            stadls::vx::v3::ContainerTicket> const& tickets) {
				        m_local_data_output.read_ppu_symbols.at(
				            batch_index)[m_execution_instance][name] =
				            std::map<
				                halco::hicann_dls::vx::v3::HemisphereOnDLS,
				                haldls::vx::v3::PPUMemoryBlock>{
				                {halco::hicann_dls::vx::v3::HemisphereOnDLS::top,
				                 dynamic_cast<haldls::vx::v3::PPUMemoryBlock const&>(
				                     tickets.at(halco::hicann_dls::vx::v3::HemisphereOnDLS::top)
				                         .get())},
				                {halco::hicann_dls::vx::v3::HemisphereOnDLS::bottom,
				                 dynamic_cast<haldls::vx::v3::PPUMemoryBlock const&>(
				                     tickets.at(halco::hicann_dls::vx::v3::HemisphereOnDLS::bottom)
				                         .get())}};
			        },
			        [&](stadls::vx::v3::ContainerTicket const& ticket) {
				        m_local_data_output.read_ppu_symbols.resize(m_batch_entries.size());
				        m_local_data_output.read_ppu_symbols.at(batch_index)
				            .at(m_execution_instance)[name] =
				            dynamic_cast<lola::vx::v3::ExternalPPUMemoryBlock const&>(ticket.get());
			        },
			    },
			    ticket);
		}
		if (batch_entry.m_ticket_events_begin && batch_entry.m_ticket_events_end) {
			total_realtime_duration += std::chrono::nanoseconds(static_cast<size_t>(
			    static_cast<double>(
			        batch_entry.m_ticket_events_end->get_fpga_time().value() -
			        batch_entry.m_ticket_events_begin->get_fpga_time().value()) /
			    static_cast<double>(haldls::vx::v3::Timer::Value::fpga_clock_cycles_per_us) *
			    1000.));
		}
	}
	signal_flow::ExecutionTimeInfo execution_time_info;
	execution_time_info.realtime_duration_per_execution_instance[m_execution_instance] =
	    total_realtime_duration;
	m_local_data_output.execution_time_info = execution_time_info;

	m_postprocessing = true;
	for (auto const vertex : m_post_vertices) {
		std::visit(
		    [&](auto const& value) {
			    hate::Timer timer;
			    process(vertex, value);
			    LOG4CXX_TRACE(
			        logger, "post_process(): Postprocessed "
			                    << name<hate::remove_all_qualifiers_t<decltype(value)>>() << " in "
			                    << timer.print() << ".");
		    },
		    m_graph.get_vertex_property(vertex));
	}
	m_postprocessing = false;

	return std::move(m_local_data_output);
}

ExecutionInstanceBuilder::Ret ExecutionInstanceBuilder::generate(ExecutionInstanceBuilder::Usages before, ExecutionInstanceBuilder::Usages after)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3;
	using namespace lola::vx::v3;

	// if no on-chip computation is to be done, return without static configuration
	auto const has_computation =
	    m_event_input_vertex.has_value() ||
	    std::any_of(
	        m_input_list.runtime.begin(), m_input_list.runtime.end(), [this](auto const& r) {
		        return r.contains(m_execution_instance) && r.at(m_execution_instance) != 0;
	        });
	if (!has_computation) {
		PlaybackProgramBuilder builder;
		builder.merge_back(m_playback_hooks.pre_static_config);
		builder.merge_back(m_playback_hooks.pre_realtime);
		builder.merge_back(m_playback_hooks.inside_realtime_begin);
		builder.merge_back(m_playback_hooks.inside_realtime.done());
		builder.merge_back(m_playback_hooks.inside_realtime_end);
		builder.merge_back(m_playback_hooks.post_realtime);
		PlaybackProgramBuilder empty_PPB;
		std::vector<ExecutionInstanceBuilder::RealtimeSnippet> empty_realtime_column;
		return {
		    std::move(empty_PPB), std::move(builder), std::move(empty_realtime_column),
		    std::move(empty_PPB)};
	}

	// absolute time playback builder sequence to be concatenated in the end
	std::vector<ExecutionInstanceBuilder::RealtimeSnippet> realtimes;

	// generate playback snippet for neuron resets
	auto builder_neuron_reset = stadls::vx::generate(m_neuron_resets);

	auto const enable_ppu = static_cast<bool>(m_ppu_symbols);
	bool const has_cadc_readout = std::any_of(
	    m_ticket_requests.begin(), m_ticket_requests.end(), [](auto const v) { return v; });
	assert(has_cadc_readout ? enable_ppu : true);

	if (has_cadc_readout) {
		assert(m_cadc_readout_mode);
		if ((*m_cadc_readout_mode ==
		     signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic) &&
		    m_has_plasticity_rule) {
			throw std::runtime_error(
			    "Periodic CADC readout and plasticity rule execution are mutually exclusive.");
		}
	}

	// look-up PPU-program symbols
	PPUMemoryBlockOnPPU ppu_result_coord;
	PPUMemoryWordOnPPU ppu_runtime_coord;
	PPUMemoryWordOnPPU ppu_status_coord;
	stadls::vx::PlaybackGeneratorReturn<AbsoluteTimePlaybackProgramBuilder, Timer::Value>
	    ppu_command_baseline_read;
	stadls::vx::PlaybackGeneratorReturn<AbsoluteTimePlaybackProgramBuilder, Timer::Value>
	    ppu_command_reset_neurons;
	stadls::vx::PlaybackGeneratorReturn<AbsoluteTimePlaybackProgramBuilder, Timer::Value>
	    ppu_command_read;
	stadls::vx::PlaybackGeneratorReturn<AbsoluteTimePlaybackProgramBuilder, Timer::Value>
	    ppu_command_stop_periodic_read;
	stadls::vx::PlaybackGeneratorReturn<AbsoluteTimePlaybackProgramBuilder, Timer::Value>
	    ppu_command_scheduler;
	PlaybackProgramBuilder wait_for_ppu_command_idle;
	std::optional<PPUMemoryBlockOnPPU> ppu_scheduler_event_drop_count_coord;
	typed_array<std::optional<ExternalPPUMemoryBlockOnFPGA>, PPUOnDLS>
	    ppu_periodic_cadc_readout_samples_coord;
	std::vector<PPUMemoryBlockOnPPU> ppu_timer_event_drop_count_coord;
	std::map<
	    signal_flow::Graph::vertex_descriptor, typed_array<ExternalPPUMemoryBlockOnFPGA, PPUOnDLS>>
	    ppu_plasticity_rule_recorded_scratchpad_memory_coord;
	if (enable_ppu) {
		assert(m_ppu_symbols);
		ppu_status_coord =
		    std::get<PPUMemoryBlockOnPPU>(m_ppu_symbols->at("status").coordinate).toMin();
		ppu_result_coord =
		    std::get<PPUMemoryBlockOnPPU>(m_ppu_symbols->at("cadc_result").coordinate);
		ppu_runtime_coord =
		    std::get<PPUMemoryBlockOnPPU>(m_ppu_symbols->at("runtime").coordinate).toMin();
		if (m_ppu_symbols->contains("scheduler_event_drop_count")) {
			ppu_scheduler_event_drop_count_coord.emplace(std::get<PPUMemoryBlockOnPPU>(
			    m_ppu_symbols->at("scheduler_event_drop_count").coordinate));
			for (auto const& [name, symbol] : *m_ppu_symbols) {
				if (name.starts_with("timer") && name.ends_with("event_drop_count")) {
					ppu_timer_event_drop_count_coord.push_back(
					    std::get<PPUMemoryBlockOnPPU>(symbol.coordinate));
				}
			}
		}
		for (auto const& [descriptor, _] :
		     m_batch_entries.at(0).m_plasticity_rule_recorded_scratchpad_memory) {
			ppu_plasticity_rule_recorded_scratchpad_memory_coord[descriptor][PPUOnDLS::top] =
			    std::get<ExternalPPUMemoryBlockOnFPGA>(
			        m_ppu_symbols
			            ->at("recorded_scratchpad_memory_top_" + std::to_string(descriptor))
			            .coordinate);
			ppu_plasticity_rule_recorded_scratchpad_memory_coord[descriptor][PPUOnDLS::bottom] =
			    std::get<ExternalPPUMemoryBlockOnFPGA>(
			        m_ppu_symbols
			            ->at("recorded_scratchpad_memory_bot_" + std::to_string(descriptor))
			            .coordinate);
		}

		if (has_cadc_readout) {
			assert(m_cadc_readout_mode);
			if (*m_cadc_readout_mode ==
			    signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic) {
				ppu_periodic_cadc_readout_samples_coord[PPUOnDLS::top] =
				    std::get<ExternalPPUMemoryBlockOnFPGA>(
				        m_ppu_symbols->at("periodic_cadc_samples_top").coordinate);
				ppu_periodic_cadc_readout_samples_coord[PPUOnDLS::bottom] =
				    std::get<ExternalPPUMemoryBlockOnFPGA>(
				        m_ppu_symbols->at("periodic_cadc_samples_bot").coordinate);
			}
		}

		// generate absolute time playback snippets for PPU command polling
		ppu_command_baseline_read = stadls::vx::generate(
		    generator::PPUCommand(ppu_status_coord, ppu::detail::Status::baseline_read));
		ppu_command_reset_neurons = stadls::vx::generate(
		    generator::PPUCommand(ppu_status_coord, ppu::detail::Status::reset_neurons));
		ppu_command_read = stadls::vx::generate(
		    generator::PPUCommand(ppu_status_coord, ppu::detail::Status::read));
		ppu_command_stop_periodic_read = stadls::vx::generate(
		    generator::PPUCommand(ppu_status_coord, ppu::detail::Status::stop_periodic_read));
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			ppu_command_scheduler.builder.write(
			    ppu_command_scheduler.result, PPUMemoryWordOnDLS(ppu_status_coord, ppu),
			    PPUMemoryWord(
			        PPUMemoryWord::Value(static_cast<uint32_t>(ppu::detail::Status::scheduler))));
			ppu_command_scheduler.result += Timer::Value(2);
			PollingOmnibusBlockConfig config;
			config.set_address(PPUMemoryWord::addresses<PollingOmnibusBlockConfig::Address>(
			                       PPUMemoryWordOnDLS(ppu_status_coord, ppu))
			                       .at(0));
			config.set_target(
			    PollingOmnibusBlockConfig::Value(static_cast<uint32_t>(ppu::detail::Status::idle)));
			config.set_mask(PollingOmnibusBlockConfig::Value(0xffffffff));
			wait_for_ppu_command_idle.write(PollingOmnibusBlockConfigOnFPGA(), config);
			wait_for_ppu_command_idle.block_until(BarrierOnFPGA(), Barrier::omnibus);
			wait_for_ppu_command_idle.block_until(
			    PollingOmnibusBlockOnFPGA(), PollingOmnibusBlock());
		}
	}

	// arm MADC
	PlaybackProgramBuilder madc_arm;
	if (m_madc_readout_vertex) {
		madc_arm = stadls::vx::generate(generator::MADCArm()).builder.done();
		madc_arm.write(TimerOnDLS(), Timer());
		madc_arm.block_until(
		    TimerOnDLS(), Timer::Value(1000 * Timer::Value::fpga_clock_cycles_per_us));
	}

	// bring PPUs in running state (start PPUs)
	PlaybackProgramBuilder trigger_builder;
	if (m_ppu_symbols) {
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			haldls::vx::v3::PPUControlRegister ctrl;
			ctrl.set_inhibit_reset(true);
			trigger_builder.write(ppu.toPPUControlRegisterOnDLS(), ctrl);
		}
		auto const ppu_status_coord =
		    std::get<PPUMemoryBlockOnPPU>(m_ppu_symbols->at("status").coordinate).toMin();
		// wait for PPUs to be ready
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			using namespace haldls::vx::v3;
			PollingOmnibusBlockConfig config;
			config.set_address(PPUMemoryWord::addresses<PollingOmnibusBlockConfig::Address>(
			                       PPUMemoryWordOnDLS(ppu_status_coord, ppu))
			                       .at(0));
			config.set_target(
			    PollingOmnibusBlockConfig::Value(static_cast<uint32_t>(ppu::detail::Status::idle)));
			config.set_mask(PollingOmnibusBlockConfig::Value(0xffffffff));
			trigger_builder.write(PollingOmnibusBlockConfigOnFPGA(), config);
			trigger_builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
			trigger_builder.block_until(PollingOmnibusBlockOnFPGA(), PollingOmnibusBlock());
		}
	}

	for (size_t b = 0; b < m_batch_entries.size(); ++b) {
		AbsoluteTimePlaybackProgramBuilder builder;
		Timer::Value current_time = Timer::Value(0);
		PlaybackProgramBuilder ppu_finish_builder;
		auto& batch_entry = m_batch_entries.at(b);
		// enable event recording
		if ((m_event_output_vertex || m_madc_readout_vertex) && !before.event_recording) {
			EventRecordingConfig config;
			config.set_enable_event_recording(true);
			builder.write(current_time, EventRecordingConfigOnFPGA(), config);
			current_time += Timer::Value(2);
		}
		// set runtime on PPU
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			if (!m_input_list.runtime.empty()) {
				// TODO (Issue #3993): Implement calculation of PPU clock freuqency vs. FPGA
				// frequency
				builder.write(
				    current_time, PPUMemoryWordOnDLS(ppu_runtime_coord, ppu),
				    PPUMemoryWord(PPUMemoryWord::Value(
				        m_input_list.runtime.at(b).at(m_execution_instance) * 2)));
			} else {
				builder.write(
				    current_time, PPUMemoryWordOnDLS(ppu_runtime_coord, ppu),
				    PPUMemoryWord(PPUMemoryWord::Value(0)));
			}
			current_time += Timer::Value(2);
		}
		// start MADC
		if (m_madc_readout_vertex && !before.madc_recording) {
			stadls::vx::PlaybackGeneratorReturn<AbsoluteTimePlaybackProgramBuilder, Timer::Value>
			    madc_start = stadls::vx::generate(generator::MADCStart());
			madc_start.builder += current_time;
			builder.merge(madc_start.builder);
			current_time += madc_start.result;
			current_time += Timer::Value(10 * Timer::Value::fpga_clock_cycles_per_us);
		}
		// cadc baseline read
		if (has_cadc_readout && enable_cadc_baseline) {
			assert(m_cadc_readout_mode);
			if (*m_cadc_readout_mode == signal_flow::vertex::CADCMembraneReadoutView::Mode::hagen) {
				builder.merge(ppu_command_baseline_read.builder + current_time);
				current_time += ppu_command_baseline_read.result;
				current_time += Timer::Value(20 * Timer::Value::fpga_clock_cycles_per_us);
			}
		}
		// reset neurons
		if (enable_ppu) {
			builder.merge(ppu_command_reset_neurons.builder + current_time);
			current_time += ppu_command_reset_neurons.result;
			current_time += Timer::Value(4 * Timer::Value::fpga_clock_cycles_per_us);
		} else {
			builder.merge(builder_neuron_reset.builder + current_time);
			current_time += builder_neuron_reset.result;
		}
		// start periodic CADC readout
		if (has_cadc_readout) {
			assert(m_cadc_readout_mode);
			if (*m_cadc_readout_mode ==
			    signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic) {
				for (auto const ppu : iter_all<PPUOnDLS>()) {
					builder.write(
					    current_time, PPUMemoryWordOnDLS(ppu_status_coord, ppu),
					    PPUMemoryWord(PPUMemoryWord::Value(
					        static_cast<uint32_t>(ppu::detail::Status::periodic_read))));
					current_time += Timer::Value(2);
				}
			}
			current_time += Timer::Value(6 * Timer::Value::fpga_clock_cycles_per_us);
		}
		// wait for membrane to settle
		if (!builder.empty()) {
			current_time += Timer::Value::fpga_clock_cycles_per_us;
		}
		Timer::Value pre_realtime_duration = current_time;
		// send input
		if (m_event_output_vertex || m_madc_readout_vertex) {
			batch_entry.m_ticket_events_begin =
			    builder.read(current_time, NullPayloadReadableOnFPGA());
			current_time += Timer::Value(1);
		}
		// trigger PPU scheduler
		if (enable_ppu && m_has_plasticity_rule) {
			builder.merge(ppu_command_scheduler.builder + current_time);
			current_time += ppu_command_scheduler.result;
		}
		// insert events of realtime section
		Timer::Value inside_realtime_duration(0);
		if (m_event_input_vertex) {
			generator::TimedSpikeToChipSequence event_generator(
			    std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
			        m_local_data.data.at(*m_event_input_vertex))
			        .at(b));
			stadls::vx::PlaybackGeneratorReturn<AbsoluteTimePlaybackProgramBuilder, Timer::Value>
			    events = stadls::vx::generate(event_generator);
			// assume, that inside_realtime hook doesn't exceed TimedSpikeToChipSequence in time
			if ((m_batch_entries.size() == 1) || (b == m_batch_entries.size() - 1)) {
				events.builder.merge(m_playback_hooks.inside_realtime);
			} else {
				events.builder.copy(m_playback_hooks.inside_realtime);
			}
			events.builder += current_time;
			builder.merge(events.builder);
			inside_realtime_duration = events.result;
		}
		// wait until runtime reached
		if (!m_input_list.runtime.empty() &&
		    m_input_list.runtime.at(b).contains(m_execution_instance)) {
			inside_realtime_duration = std::max(
			    inside_realtime_duration,
			    m_input_list.runtime.at(b).at(m_execution_instance).toTimerOnFPGAValue());
		}
		current_time += inside_realtime_duration;
		if (m_event_output_vertex || m_madc_readout_vertex) {
			batch_entry.m_ticket_events_end =
			    builder.read(current_time, NullPayloadReadableOnFPGA());
			current_time += Timer::Value(1);
		}
		// wait for membrane to settle
		if (!builder.empty()) {
			current_time += Timer::Value::fpga_clock_cycles_per_us;
		}
		// read out neuron membranes
		if (has_cadc_readout) {
			assert(m_cadc_readout_mode);
			if (*m_cadc_readout_mode == signal_flow::vertex::CADCMembraneReadoutView::Mode::hagen) {
				builder.merge(ppu_command_read.builder + current_time);
				current_time += ppu_command_read.result;
				// readout result
				current_time += Timer::Value(Timer::Value::fpga_clock_cycles_per_us * 10);
				for (auto const ppu : iter_all<PPUOnDLS>()) {
					batch_entry.m_ppu_result[ppu] =
					    builder.read(current_time, PPUMemoryBlockOnDLS(ppu_result_coord, ppu));
					current_time += Timer::Value(128);
				}
			}
		}
		// stop periodic CADC readout
		if (has_cadc_readout) {
			assert(m_cadc_readout_mode);
			if (*m_cadc_readout_mode ==
			    signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic) {
				builder.copy(ppu_command_stop_periodic_read.builder + current_time);
				current_time += ppu_command_stop_periodic_read.result;
			}
		}
		// stop MADC (and power-down in last batch entry)
		if (m_madc_readout_vertex && !after.madc_recording) {
			stadls::vx::PlaybackGeneratorReturn<AbsoluteTimePlaybackProgramBuilder, Timer::Value>
			    madc_stop = stadls::vx::generate(generator::MADCStop{});
			madc_stop.builder += current_time;
			builder.merge(madc_stop.builder);
			current_time += madc_stop.result;
		}
		// disable event recording
		if ((m_event_output_vertex || m_madc_readout_vertex) && !after.event_recording) {
			EventRecordingConfig config;
			config.set_enable_event_recording(false);
			builder.write(current_time, EventRecordingConfigOnFPGA(), config);
			current_time += Timer::Value(2);
		}
		// readout extmem data from periodic CADC readout
		if (has_cadc_readout) {
			assert(m_cadc_readout_mode);
			if (*m_cadc_readout_mode ==
			    signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic) {
				InstructionTimeoutConfig instruction_timeout;
				instruction_timeout.set_value(InstructionTimeoutConfig::Value(
				    100000 * InstructionTimeoutConfig::Value::fpga_clock_cycles_per_us));
				ppu_finish_builder.write(
				    halco::hicann_dls::vx::InstructionTimeoutConfigOnFPGA(), instruction_timeout);
				ppu_finish_builder.copy_back(wait_for_ppu_command_idle);
				ppu_finish_builder.write(
				    halco::hicann_dls::vx::InstructionTimeoutConfigOnFPGA(),
				    InstructionTimeoutConfig());
				for (auto const ppu : iter_all<PPUOnDLS>()) {
					if (!m_ticket_requests[ppu.toHemisphereOnDLS()]) {
						continue;
					}
					assert(ppu_periodic_cadc_readout_samples_coord[ppu]);

					// calculate aproximate number of expected samples
					size_t expected_size = 0;
					// lower bound on sample duration -> upper bound on sample rate
					size_t const approx_sample_duration =
					    static_cast<size_t>(1.7 * Timer::Value::fpga_clock_cycles_per_us);
					// get experiment duration either via runtime or via last spike time
					if (!m_input_list.runtime.empty()) {
						expected_size = m_input_list.runtime.at(b).at(m_execution_instance) /
						                approx_sample_duration;
					} else if (m_event_input_vertex) {
						auto const& spikes =
						    std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
						        m_local_data.data.at(*m_event_input_vertex))
						        .at(b);
						if (!spikes.empty()) {
							expected_size = spikes.back().time / approx_sample_duration;
						}
					}
					// add a sample as constant margin for error
					expected_size += 1;
					// cap at maximal possible amount of samples
					expected_size = std::min(expected_size, static_cast<size_t>(100));

					batch_entry.m_extmem_result[ppu] =
					    ppu_finish_builder.read(ExternalPPUMemoryBlockOnFPGA(
					        ppu_periodic_cadc_readout_samples_coord[ppu]->toMin(),
					        ExternalPPUMemoryByteOnFPGA(
					            ppu_periodic_cadc_readout_samples_coord[ppu]->toMin() + 128 +
					            ((256 + 128) * expected_size) - 1)));
				}
			}
		}
		// wait until scheduler finished, readout stats
		if (enable_ppu && m_has_plasticity_rule) {
			// increase instruction timeout for wait until scheduler finished
			InstructionTimeoutConfig instruction_timeout;
			instruction_timeout.set_value(InstructionTimeoutConfig::Value(
			    100000 * InstructionTimeoutConfig::Value::fpga_clock_cycles_per_us));
			ppu_finish_builder.write(
			    halco::hicann_dls::vx::InstructionTimeoutConfigOnFPGA(), instruction_timeout);
			// wait for PPUs being idle
			ppu_finish_builder.copy_back(wait_for_ppu_command_idle);
			// reset instruction timeout to default after wait until scheduler finished
			ppu_finish_builder.write(
			    halco::hicann_dls::vx::InstructionTimeoutConfigOnFPGA(),
			    InstructionTimeoutConfig());
			// readout stats
			for (auto const ppu : iter_all<PPUOnDLS>()) {
				batch_entry.m_ppu_scheduler_finished[ppu] =
				    ppu_finish_builder.read(PPUMemoryBlockOnDLS(
				        PPUMemoryBlockOnPPU(ppu_status_coord, ppu_status_coord), ppu));
			}
			if (ppu_scheduler_event_drop_count_coord) {
				for (auto const ppu : iter_all<PPUOnDLS>()) {
					batch_entry.m_ppu_scheduler_event_drop_count[ppu] = ppu_finish_builder.read(
					    PPUMemoryBlockOnDLS(*ppu_scheduler_event_drop_count_coord, ppu));
				}
			}
			batch_entry.m_ppu_timer_event_drop_count.resize(
			    ppu_timer_event_drop_count_coord.size());
			size_t i = 0;
			for (auto const& coord : ppu_timer_event_drop_count_coord) {
				for (auto const ppu : iter_all<PPUOnDLS>()) {
					batch_entry.m_ppu_timer_event_drop_count.at(i)[ppu] =
					    ppu_finish_builder.read(PPUMemoryBlockOnDLS(coord, ppu));
				}
				i++;
			}
			for (auto const ppu : iter_all<PPUOnDLS>()) {
				batch_entry.m_ppu_mailbox[ppu] =
				    ppu_finish_builder.read(PPUMemoryBlockOnDLS(PPUMemoryBlockOnPPU::mailbox, ppu));
			}
		}
		// readout recorded scratchpad memory for plasticity rules
		if (m_has_plasticity_rule) {
			for (auto const& [descriptor, coords] :
			     ppu_plasticity_rule_recorded_scratchpad_memory_coord) {
				for (auto const ppu : iter_all<PPUOnDLS>()) {
					batch_entry.m_plasticity_rule_recorded_scratchpad_memory.at(descriptor)[ppu] =
					    ppu_finish_builder.read(coords[ppu]);
				}
			}
		}
		// readout requested PPU symbols
		for (auto const& name : m_playback_hooks.read_ppu_symbols) {
			if (!m_ppu_symbols) {
				throw std::runtime_error("Provided PPU symbols but not PPU program is present.");
			}
			if (!m_ppu_symbols->contains(name)) {
				throw std::runtime_error(
				    "Provided unknown symbol name via ExecutionInstancePlaybackHooks.");
			}
			std::visit(
			    hate::overloaded{
			        [&](halco::hicann_dls::vx::v3::PPUMemoryBlockOnPPU const& coordinate) {
				        std::map<HemisphereOnDLS, ContainerTicket> values;
				        values.emplace(
				            HemisphereOnDLS::top, ppu_finish_builder.read(PPUMemoryBlockOnDLS(
				                                      coordinate, PPUOnDLS::top)));
				        values.emplace(
				            HemisphereOnDLS::bottom, ppu_finish_builder.read(PPUMemoryBlockOnDLS(
				                                         coordinate, PPUOnDLS::bottom)));
				        batch_entry.ppu_symbols[name] = values;
			        },
			        [&](halco::hicann_dls::vx::v3::ExternalPPUMemoryBlockOnFPGA const& coordinate) {
				        batch_entry.ppu_symbols.insert({name, ppu_finish_builder.read(coordinate)});
			        }},
			    m_ppu_symbols->at(name).coordinate);
			for (auto const& [descriptor, coords] :
			     ppu_plasticity_rule_recorded_scratchpad_memory_coord) {
				for (auto const ppu : iter_all<PPUOnDLS>()) {
					batch_entry.m_plasticity_rule_recorded_scratchpad_memory.at(descriptor)[ppu] =
					    ppu_finish_builder.read(coords[ppu]);
				}
			}
		}
		realtimes.push_back(ExecutionInstanceBuilder::RealtimeSnippet{
		    .builder = std::move(builder),
		    .ppu_finish_builder = std::move(ppu_finish_builder),
		    .pre_realtime_duration = pre_realtime_duration,
		    .realtime_duration = current_time - pre_realtime_duration});
	}

	// stop PPUs
	PlaybackProgramBuilder builder_stopPPU;
	if (enable_ppu) {
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			PPUMemoryWord config(
			    PPUMemoryWord::Value(static_cast<uint32_t>(ppu::detail::Status::stop)));
			builder_stopPPU.write(PPUMemoryWordOnDLS(ppu_status_coord, ppu), config);
		}
		// poll for completion by waiting until PPU is asleep
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			PollingOmnibusBlockConfig config;
			config.set_address(
			    PPUStatusRegister::read_addresses<PollingOmnibusBlockConfig::Address>(
			        ppu.toPPUStatusRegisterOnDLS())
			        .at(0));
			config.set_target(PollingOmnibusBlockConfig::Value(static_cast<uint32_t>(true)));
			config.set_mask(PollingOmnibusBlockConfig::Value(0x00000001));
			builder_stopPPU.write(PollingOmnibusBlockConfigOnFPGA(), config);
			builder_stopPPU.block_until(BarrierOnFPGA(), Barrier::omnibus);
			builder_stopPPU.block_until(PollingOmnibusBlockOnFPGA(), PollingOmnibusBlock());
		}
		// disable inhibit reset
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			PPUControlRegister ctrl;
			ctrl.set_inhibit_reset(false);
			builder_stopPPU.write(ppu.toPPUControlRegisterOnDLS(), ctrl);
		}
		builder_stopPPU.block_until(BarrierOnFPGA(), Barrier::omnibus);
	}
	return ExecutionInstanceBuilder::Ret{
	    .start_ppu = std::move(trigger_builder),
	    .arm_madc = std::move(madc_arm),
	    .realtimes = std::move(realtimes),
	    .stop_ppu = std::move(builder_stopPPU)};
}

} // namespace grenade::vx::execution::detail
