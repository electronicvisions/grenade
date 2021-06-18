#include "grenade/vx/execution_instance_builder.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <boost/range/combine.hpp>
#include <boost/type_index.hpp>
#include <log4cxx/logger.h>
#include <tbb/parallel_for_each.h>

#include "grenade/vx/execution_instance.h"
#include "grenade/vx/generator/madc.h"
#include "grenade/vx/generator/ppu.h"
#include "grenade/vx/generator/timed_spike_sequence.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/status.h"
#include "grenade/vx/types.h"
#include "haldls/vx/v2/barrier.h"
#include "haldls/vx/v2/block.h"
#include "haldls/vx/v2/padi.h"
#include "hate/timer.h"
#include "hate/type_traits.h"
#include "lola/vx/ppu.h"
#include "stadls/vx/constants.h"

namespace grenade::vx {

namespace {

template <typename T>
std::string name()
{
	auto const full = boost::typeindex::type_id<T>().pretty_name();
	return full.substr(full.rfind(':') + 1);
}

} // namespace

ExecutionInstanceBuilder::ExecutionInstanceBuilder(
    Graph const& graph,
    coordinate::ExecutionInstance const& execution_instance,
    IODataMap const& input_list,
    IODataMap const& data_output,
    ChipConfig const& chip_config,
    ExecutionInstancePlaybackHooks& playback_hooks) :
    m_graph(graph),
    m_execution_instance(execution_instance),
    m_input_list(input_list),
    m_data_output(data_output),
    m_local_external_data(),
    m_config_builder(graph, execution_instance, chip_config),
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

	m_local_external_data.runtime = m_input_list.runtime;
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
		if (std::holds_alternative<vertex::ExternalInput>(m_graph.get_vertex_property(vertex))) {
			if (m_input_list.data.find(vertex) == m_input_list.data.end()) {
				return true;
			}
			if (batch_size == 0) {
				return false;
			}
			auto const& input_vertex =
			    std::get<vertex::ExternalInput>(m_graph.get_vertex_property(vertex));
			return !IODataMap::is_match(
			    m_input_list.data.find(vertex)->second, input_vertex.output());
		}
		return false;
	});
}

bool ExecutionInstanceBuilder::inputs_available(Graph::vertex_descriptor const descriptor) const
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
    Graph::vertex_descriptor const /* vertex */, T const& /* data */)
{
	// Specialize for types which are not empty
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /*vertex*/, vertex::NeuronView const& data)
{
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
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
    Graph::vertex_descriptor const vertex, vertex::CrossbarL2Input const&)
{
	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
	auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
	auto const in_vertex = boost::source(*(in_edges.first), m_graph.get_graph());

	m_local_data.data[vertex] = m_local_data.data.at(in_vertex);
	m_event_input_vertex = vertex;
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::CrossbarL2Output const&)
{
	if (!m_postprocessing) {
		if (m_event_output_vertex) {
			throw std::logic_error("Only one event output vertex allowed.");
		}
		m_event_output_vertex = vertex;
		m_post_vertices.push_back(vertex);
	} else {
		auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceBuilder");

		stadls::vx::PlaybackProgram::spikes_type spikes;
		for (auto const& program : m_chunked_program) {
			auto const local_spikes = program.get_spikes();

			LOG4CXX_INFO(logger, "process(): " << local_spikes.size() << " spikes");

			spikes.insert(spikes.end(), local_spikes.begin(), local_spikes.end());
		}
		m_local_data.data[vertex] = filter_events(spikes);
	}
}

namespace {

template <typename T>
std::vector<std::vector<T>> apply_restriction(
    std::vector<std::vector<T>> const& value, PortRestriction const& restriction)
{
	std::vector<std::vector<T>> ret(value.size());
	for (size_t b = 0; b < ret.size(); ++b) {
		auto& local_ret = ret.at(b);
		auto const& local_value = value.at(b);
		local_ret.insert(
		    local_ret.end(), local_value.begin() + restriction.min(),
		    local_value.begin() + restriction.max() + 1);
	}
	return ret;
}

} // namespace

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::DataInput const& /* data */)
{
	using namespace lola::vx::v2;
	using namespace haldls::vx::v2;
	using namespace halco::hicann_dls::vx::v2;
	using namespace halco::common;

	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
	auto const edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
	auto const in_vertex = boost::source(edge, m_graph.get_graph());
	auto const& input_values =
	    ((std::holds_alternative<vertex::ExternalInput>(m_graph.get_vertex_property(in_vertex)))
	         ? m_local_external_data.data.at(in_vertex)
	         : m_data_output.data.at(in_vertex));

	auto const maybe_apply_restriction = [&](auto const& d) -> IODataMap::Entry {
		typedef std::remove_cvref_t<decltype(d)> Data;
		typedef hate::type_list<
		    std::vector<std::vector<UInt32>>, std::vector<std::vector<UInt5>>,
		    std::vector<std::vector<Int8>>>
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
    Graph::vertex_descriptor const vertex, vertex::CADCMembraneReadoutView const& data)
{
	// CADCMembraneReadoutView inputs size equals 1
	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);

	// get source NeuronView
	auto const synram = data.get_synram();
	auto const hemisphere = synram.toHemisphereOnDLS();
	auto const& columns = data.get_columns();

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
	using namespace lola::vx::v2;
	using namespace haldls::vx::v2;
	if (!m_postprocessing) { // pre-hw-run processing
		m_ticket_requests[hemisphere] = true;
		// results need hardware execution
		m_post_vertices.push_back(vertex);
	} else { // post-hw-run processing
		// extract Int8 values
		std::vector<std::vector<Int8>> sample_batches;
		resize_rectangular(sample_batches, m_batch_entries.size(), data.output().size);
		for (size_t batch_index = 0; batch_index < m_batch_entries.size(); ++batch_index) {
			assert(m_batch_entries.at(batch_index).m_ppu_result[synram.toPPUOnDLS()]);
			auto const block =
			    m_batch_entries.at(batch_index).m_ppu_result[synram.toPPUOnDLS()]->get();
			auto const values = from_vector_unit_row(block);
			auto& samples = sample_batches.at(batch_index);

			// get samples via neuron mapping from incoming NeuronView
			size_t i = 0;
			for (auto const& column : columns) {
				samples.at(i) = Int8(values[column]);
				i++;
			}
		}
		m_local_data.data[vertex] = sample_batches;
	}
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::ExternalInput const& /* data */)
{
	auto const& input_values = m_input_list.data.at(vertex);
	m_local_external_data.data.insert({vertex, input_values});
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::Addition const& data)
{
	std::vector<std::vector<Int8>> values;
	resize_rectangular(values, m_input_list.batch_size(), data.output().size);

	std::vector<intmax_t> tmps(data.output().size, 0);
	auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
	for (size_t j = 0; j < values.size(); ++j) {
		for (auto const in_edge : boost::make_iterator_range(in_edges)) {
			auto const& local_data =
			    std::get<std::vector<std::vector<Int8>>>(
			        m_local_data.data.at(boost::source(in_edge, m_graph.get_graph())))
			        .at(j);
			assert(tmps.size() == local_data.size());
			for (size_t i = 0; i < data.output().size; ++i) {
				tmps[i] += local_data[i];
			}
		}
		// restrict to range [-128,127]
		std::transform(tmps.begin(), tmps.end(), values.at(j).begin(), [](auto const tmp) {
			return Int8(std::min(std::max(tmp, intmax_t(-128)), intmax_t(127)));
		});
		std::fill(tmps.begin(), tmps.end(), 0);
	}
	m_local_data.data[vertex] = values;
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::ArgMax const& data)
{
	// get in edge
	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
	auto const in_edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);

	auto const compute = [&](auto const& local_data) {
		// check size match only for first because we know that the data map is valid
		assert(!local_data.size() || data.inputs().front().size == local_data.front().size());
		std::vector<std::vector<UInt32>> tmps(local_data.size());
		assert(data.output().size == 1);
		for (auto& t : tmps) {
			t.resize(1 /* data.output().size */);
		}
		size_t i = 0;
		for (auto const& entry : local_data) {
			tmps.at(i).at(0) =
			    UInt32(std::distance(entry.begin(), std::max_element(entry.begin(), entry.end())));
			i++;
		}
		m_local_data.data[vertex] = tmps;
	};

	auto const visitor = [&](auto const& d) {
		typedef std::remove_cvref_t<decltype(d)> Data;
		typedef hate::type_list<
		    std::vector<std::vector<UInt32>>, std::vector<std::vector<UInt5>>,
		    std::vector<std::vector<Int8>>>
		    MatrixData;
		if constexpr (hate::is_in_type_list<Data, MatrixData>::value) {
			compute(d);
		} else {
			throw std::logic_error("ArgMax data type not implemented.");
		}
	};
	auto const& local_data = m_local_data.data.at(boost::source(in_edge, m_graph.get_graph()));
	std::visit(visitor, local_data);
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::ReLU const& data)
{
	std::vector<std::vector<Int8>> values;
	resize_rectangular(values, m_input_list.batch_size(), data.output().size);
	auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
	auto const source = boost::source(*(in_edges.first), m_graph.get_graph());
	for (size_t j = 0; j < values.size(); ++j) {
		auto const& d =
		    std::get<std::vector<std::vector<Int8>>>(m_local_data.data.at(source)).at(j);
		for (size_t i = 0; i < data.output().size; ++i) {
			values.at(j).at(i) = std::max(d.at(i), Int8(0));
		}
	}
	m_local_data.data[vertex] = values;
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::ConvertingReLU const& data)
{
	auto const shift = data.get_shift();
	std::vector<std::vector<UInt5>> values;
	resize_rectangular(values, m_input_list.batch_size(), data.output().size);
	auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
	auto const source = boost::source(*(in_edges.first), m_graph.get_graph());
	for (size_t j = 0; j < values.size(); ++j) {
		auto const& d =
		    std::get<std::vector<std::vector<Int8>>>(m_local_data.data.at(source)).at(j);
		for (size_t i = 0; i < data.output().size; ++i) {
			values.at(j).at(i) = UInt5(std::min(
			    static_cast<UInt5::value_type>(std::max(d.at(i), Int8(0)) >> shift), UInt5::max));
		}
	}
	m_local_data.data[vertex] = values;
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::DataOutput const& data)
{
	// get in edge
	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
	auto const in_edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
	auto const& local_data = m_local_data.data.at(boost::source(in_edge, m_graph.get_graph()));
	// check size match only for first because we know that the data map is valid
	assert(IODataMap::is_match(local_data, data.output()));
	m_local_data_output.data[vertex] = local_data;
}

template <typename T>
std::vector<std::vector<T>> ExecutionInstanceBuilder::filter_events(std::vector<T>& data) const
{
	// early return if no events are recorded
	if (data.empty()) {
		std::vector<std::vector<T>> data_batches(m_batch_entries.size());
		return data_batches;
	}
	// sort events by chip time
	std::sort(data.begin(), data.end(), [](auto const& a, auto const& b) {
		return a.get_chip_time() < b.get_chip_time();
	});
	// iterate over batch entries and extract associated events
	std::vector<std::vector<T>> data_batches;
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
			return event.get_chip_time().value() >= interval_begin_time;
		});
		// add empty events and continue if no spikes are recorded for this batch entry
		if (begin == data.end()) {
			data_batches.push_back({});
			continue;
		}
		// find end of interval
		auto const offset_chip_time = begin->get_chip_time();
		assert(e.m_ticket_events_end);
		auto const interval_end_time = e.m_ticket_events_end->get_fpga_time().value();
		uintmax_t end_time = 0;
		if (!m_local_external_data.runtime.empty()) {
			auto const absolute_end_chip_time =
			    m_local_external_data.runtime.at(i).value() + offset_chip_time.value();
			assert(e.m_ticket_events_end);
			end_time = std::min(interval_end_time, absolute_end_chip_time);
		} else {
			end_time = interval_end_time;
		}
		auto end = std::find_if(begin, data.end(), [&](auto const& event) {
			return event.get_chip_time().value() >= end_time;
		});
		// copy interval content
		std::vector<T> data_batch = std::vector<T>(begin, end);
		// subtract the interval begin time to get relative times
		for (auto& event : data_batch) {
			event.set_chip_time(
			    haldls::vx::v2::ChipTime(event.get_chip_time() - interval_begin_time));
		}
		data_batches.push_back(std::move(data_batch));
		begin = end;
		i++;
	}
	return data_batches;
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::MADCReadoutView const&)
{
	if (!m_postprocessing) {
		if (m_madc_readout_vertex) {
			throw std::logic_error("Only one MADC readout vertex allowed.");
		}
		m_madc_readout_vertex = vertex;
		m_post_vertices.push_back(vertex);
	} else {
		auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceBuilder");

		stadls::vx::PlaybackProgram::madc_samples_type madc_samples;
		for (auto const& program : m_chunked_program) {
			auto const local_madc_samples = program.get_madc_samples();

			LOG4CXX_INFO(logger, "process(): " << local_madc_samples.size() << " MADC samples");

			madc_samples.insert(
			    madc_samples.end(), local_madc_samples.begin(), local_madc_samples.end());
		}
		m_local_data.data[vertex] = filter_events(madc_samples);
	}
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::Transformation const& data)
{
	using namespace lola::vx::v2;
	using namespace haldls::vx::v2;
	using namespace halco::hicann_dls::vx::v2;
	using namespace halco::common;

	// fill input value
	auto const inputs = data.inputs();
	std::vector<vertex::Transformation::Function::Value> value_input;
	auto edge_it = boost::in_edges(vertex, m_graph.get_graph()).first;
	for (auto const& port : inputs) {
		if (m_graph.get_edge_property_map().at(*edge_it)) {
			throw std::logic_error("Edge with port restriction unsupported.");
		}
		auto const in_vertex = boost::source(*edge_it, m_graph.get_graph());
		auto const& input_values =
		    ((std::holds_alternative<vertex::ExternalInput>(m_graph.get_vertex_property(in_vertex)))
		         ? m_local_external_data.data.at(in_vertex)
		         : m_local_data.data.at(in_vertex));
		if (!IODataMap::is_match(input_values, port)) {
			throw std::runtime_error("Data size does not match expectation.");
		}
		value_input.push_back(input_values);
		edge_it++;
	}
	// execute transformation
	auto const value_output = data.apply(value_input);
	// process output value
	if (!IODataMap::is_match(value_output, data.output())) {
		throw std::runtime_error("Data size does not match expectation.");
	}
	m_local_data.data[vertex] = value_output;
}

void ExecutionInstanceBuilder::pre_process()
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceBuilder");
	auto const execution_instance_vertex =
	    m_graph.get_execution_instance_map().right.at(m_execution_instance);
	// Sequential preprocessing because vertices might depend on each other.
	// This is only the case for DataInput -> SynapseArrayView with HAGEN-bug workaround enabled
	// currently.
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
	m_config_builder.pre_process();
}

IODataMap ExecutionInstanceBuilder::post_process()
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceBuilder");

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

std::vector<stadls::vx::v2::PlaybackProgram> ExecutionInstanceBuilder::generate()
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
	using namespace stadls::vx::v2;
	using namespace lola::vx::v2;

	// if no on-chip computation is to be done, return without static configuration
	auto const has_computation =
	    m_event_input_vertex.has_value() ||
	    std::any_of(
	        m_local_external_data.runtime.begin(), m_local_external_data.runtime.end(),
	        [](auto const& r) { return r != 0; });
	if (!has_computation) {
		PlaybackProgramBuilder builder;
		builder.merge_back(m_playback_hooks.pre_static_config);
		builder.merge_back(m_playback_hooks.pre_realtime);
		builder.merge_back(m_playback_hooks.post_realtime);
		m_chunked_program = {builder.done()};
		return m_chunked_program;
	}

	// playback builder sequence to be concatenated in the end
	std::vector<PlaybackProgramBuilder> builders;

	// add pre static config playback hook
	builders.push_back(std::move(m_playback_hooks.pre_static_config));

	// generate static configuration
	auto [config_builder, ppu_symbols] = m_config_builder.generate();

	// add static config
	builders.push_back(std::move(config_builder));

	// generate playback snippet for neuron resets
	auto [builder_neuron_reset, _] = stadls::vx::generate(m_neuron_resets);

	// look-up PPU-program symbols
	bool const enable_ppu = static_cast<bool>(ppu_symbols);
	PPUMemoryBlockOnPPU ppu_result_coord;
	PlaybackProgramBuilder blocking_ppu_command_baseline_read;
	PlaybackProgramBuilder blocking_ppu_command_reset_neurons;
	PlaybackProgramBuilder blocking_ppu_command_read;
	if (enable_ppu) {
		assert(ppu_symbols);
		auto const ppu_status_coord = ppu_symbols->at("status").coordinate.toMin();
		ppu_result_coord = ppu_symbols->at("cadc_result").coordinate;

		// generate playback snippets for PPU command polling
		blocking_ppu_command_baseline_read =
		    stadls::vx::generate(
		        generator::BlockingPPUCommand(ppu_status_coord, ppu::Status::baseline_read))
		        .builder;
		blocking_ppu_command_reset_neurons =
		    stadls::vx::generate(
		        generator::BlockingPPUCommand(ppu_status_coord, ppu::Status::reset_neurons))
		        .builder;
		blocking_ppu_command_read =
		    stadls::vx::generate(generator::BlockingPPUCommand(ppu_status_coord, ppu::Status::read))
		        .builder;
	}

	// add pre realtime playback hook
	builders.push_back(std::move(m_playback_hooks.pre_realtime));

	bool const has_cadc_readout = std::any_of(
	    m_ticket_requests.begin(), m_ticket_requests.end(), [](auto const v) { return v; });
	assert(has_cadc_readout ? enable_ppu : true);

	for (size_t b = 0; b < m_batch_entries.size(); ++b) {
		PlaybackProgramBuilder builder;
		auto& batch_entry = m_batch_entries.at(b);
		// start MADC
		if (m_madc_readout_vertex) {
			builder.merge_back(stadls::vx::generate(generator::MADCArm()).builder);
			builder.merge_back(stadls::vx::generate(generator::MADCStart()).builder);
		}
		// cadc baseline read
		if (has_cadc_readout && enable_cadc_baseline) {
			builder.copy_back(blocking_ppu_command_baseline_read);
		}
		// reset neurons
		if (enable_ppu) {
			builder.copy_back(blocking_ppu_command_reset_neurons);
		} else {
			builder.copy_back(builder_neuron_reset);
			builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
		}
		// wait for membrane to settle
		if (!builder.empty()) {
			builder.write(TimerOnDLS(), Timer());
			builder.block_until(TimerOnDLS(), Timer::Value::fpga_clock_cycles_per_us);
		}
		// send input
		if (m_event_output_vertex || m_madc_readout_vertex) {
			batch_entry.m_ticket_events_begin = builder.read(NullPayloadReadableOnFPGA());
		}
		if (m_event_input_vertex) {
			generator::TimedSpikeSequence event_generator(
			    std::get<std::vector<TimedSpikeSequence>>(
			        m_local_data.data.at(*m_event_input_vertex))
			        .at(b));
			auto [builder_events, _] = stadls::vx::generate(event_generator);
			builder.merge_back(builder_events);
		}
		// wait until runtime reached
		if (!m_local_external_data.runtime.empty()) {
			builder.block_until(TimerOnDLS(), m_local_external_data.runtime.at(b));
		}
		if (m_event_output_vertex || m_madc_readout_vertex) {
			batch_entry.m_ticket_events_end = builder.read(NullPayloadReadableOnFPGA());
		}
		// wait for membrane to settle
		if (!builder.empty()) {
			builder.write(halco::hicann_dls::vx::TimerOnDLS(), haldls::vx::v2::Timer());
			builder.block_until(
			    halco::hicann_dls::vx::TimerOnDLS(),
			    haldls::vx::v2::Timer::Value(
			        1.0 * haldls::vx::v2::Timer::Value::fpga_clock_cycles_per_us));
		}
		// read out neuron membranes
		if (has_cadc_readout) {
			builder.copy_back(blocking_ppu_command_read);
			// readout result
			for (auto const ppu : iter_all<PPUOnDLS>()) {
				batch_entry.m_ppu_result[ppu] =
				    builder.read(PPUMemoryBlockOnDLS(ppu_result_coord, ppu));
			}
		}
		// stop MADC
		if (m_madc_readout_vertex) {
			builder.merge_back(stadls::vx::generate(generator::MADCStop()).builder);
		}
		// wait for response data
		if (!builder.empty()) {
			builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
		}
		builders.push_back(std::move(builder));
	}

	// add post realtime playback hook
	builders.push_back(std::move(m_playback_hooks.post_realtime));

	// Merge builders sequentially into chunks smaller than the FPGA playback memory size.
	// If a single builder is larger than the memory, it is placed isolated in a program.
	PlaybackProgramBuilder builder;
	for (auto& b : builders) {
		if ((builder.size_to_fpga() + b.size_to_fpga()) >
		    stadls::vx::playback_memory_size_to_fpga) {
			m_chunked_program.push_back(builder.done());
		}
		builder.merge_back(b);
	}
	if (!builder.empty()) {
		m_chunked_program.push_back(builder.done());
	}
	return m_chunked_program;
}

} // namespace grenade::vx
