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
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/status.h"
#include "grenade/vx/types.h"
#include "haldls/vx/v2/barrier.h"
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
    ChipConfig const& chip_config) :
    m_graph(graph),
    m_execution_instance(execution_instance),
    m_input_list(input_list),
    m_data_output(data_output),
    m_local_external_data(),
    m_config_builder(graph, execution_instance, chip_config),
    m_post_vertices(),
    m_local_data(),
    m_local_data_output()
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
		return false;
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
			auto const& input_vertex =
			    std::get<vertex::ExternalInput>(m_graph.get_vertex_property(vertex));
			if (input_vertex.output().type == ConnectionType::DataUInt32) {
				if (m_input_list.uint32.find(vertex) == m_input_list.uint32.end()) {
					return true;
				} else if (
				    (batch_size != 0) &&
				    m_input_list.uint32.at(vertex).front().size() != input_vertex.output().size) {
					return true;
				}
			} else if (input_vertex.output().type == ConnectionType::DataUInt5) {
				if (m_input_list.uint5.find(vertex) == m_input_list.uint5.end()) {
					// incomplete because value not found
					return true;
				} else if (
				    (batch_size != 0) &&
				    m_input_list.uint5.at(vertex).front().size() != input_vertex.output().size) {
					// incomplete because value size doesn't match expectation
					return true;
				}
			} else if (input_vertex.output().type == ConnectionType::DataInt8) {
				if (m_input_list.int8.find(vertex) == m_input_list.int8.end()) {
					return true;
				} else if (
				    (batch_size != 0) &&
				    m_input_list.int8.at(vertex).front().size() != input_vertex.output().size) {
					return true;
				}
			} else if (input_vertex.output().type == ConnectionType::DataTimedSpikeSequence) {
				if (m_input_list.spike_events.find(vertex) == m_input_list.spike_events.end()) {
					return true;
				}
			} else {
				throw std::runtime_error("ExternalInput output type not supported.");
			}
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
	auto const& enable_resets = data.get_enable_resets();
	for (auto const column : data.get_columns()) {
		auto const neuron_reset = AtomicNeuronOnDLS(column, data.get_row()).toNeuronResetOnDLS();
		m_neuron_resets.enable_resets[neuron_reset] = enable_resets.at(i);
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

	m_local_data.spike_events[vertex] = m_local_data.spike_events.at(in_vertex);
	m_event_input_vertex = vertex;
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::DataInput const& data)
{
	using namespace lola::vx::v2;
	using namespace haldls::vx::v2;
	using namespace halco::hicann_dls::vx::v2;
	using namespace halco::common;
	// TODO: reduce double code by providing get<Type>(data_map) (Issue #3717)
	if (data.output().type == ConnectionType::UInt32) {
		assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
		auto const edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
		if (m_graph.get_edge_property_map().at(edge)) {
			throw std::logic_error("Edge with port restriction unsupported.");
		}
		auto const in_vertex = boost::source(edge, m_graph.get_graph());
		auto const& input_values =
		    ((std::holds_alternative<vertex::ExternalInput>(m_graph.get_vertex_property(in_vertex)))
		         ? m_local_external_data.uint32.at(in_vertex)
		         : m_data_output.uint32.at(in_vertex));

		m_local_data.uint32[vertex] = input_values;
	} else if (data.output().type == ConnectionType::UInt5) {
		assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
		auto const edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
		if (m_graph.get_edge_property_map().at(edge)) {
			throw std::logic_error("Edge with port restriction unsupported.");
		}
		auto const in_vertex = boost::source(edge, m_graph.get_graph());
		auto const& input_values =
		    ((std::holds_alternative<vertex::ExternalInput>(m_graph.get_vertex_property(in_vertex)))
		         ? m_local_external_data.uint5.at(in_vertex)
		         : m_data_output.uint5.at(in_vertex));

		m_local_data.uint5[vertex] = input_values;
	} else if (data.output().type == ConnectionType::Int8) {
		assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
		auto const edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
		if (m_graph.get_edge_property_map().at(edge)) {
			throw std::logic_error("Edge with port restriction unsupported.");
		}
		auto const in_vertex = boost::source(edge, m_graph.get_graph());
		auto const& input_values =
		    ((std::holds_alternative<vertex::ExternalInput>(m_graph.get_vertex_property(in_vertex)))
		         ? m_local_external_data.int8.at(in_vertex)
		         : m_data_output.int8.at(in_vertex));

		m_local_data.int8[vertex] = input_values;
	} else if (data.output().type == ConnectionType::TimedSpikeSequence) {
		assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
		auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
		auto const in_vertex = boost::source(*(in_edges.first), m_graph.get_graph());

		m_local_data.spike_events[vertex] = m_local_external_data.spike_events.at(in_vertex);
	} else {
		throw std::logic_error("DataInput output connection type processing not implemented.");
	}
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
		auto& sample_batches = m_local_data.int8[vertex];
		resize_rectangular(sample_batches, m_batch_entries.size(), data.output().size);
		if (enable_ppu) {
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
		} else {
			if (enable_cadc_baseline) {
				for (size_t batch_index = 0; batch_index < m_batch_entries.size(); ++batch_index) {
					assert(m_batch_entries.at(batch_index).m_cadc_values);
					auto const& values = *(m_batch_entries.at(batch_index).m_cadc_values);
					auto const& synram_values = values.causal[synram];
					assert(m_batch_entries.at(batch_index).m_cadc_baseline_values);
					auto const& values_baseline =
					    *(m_batch_entries.at(batch_index).m_cadc_baseline_values);
					auto const& synram_values_baseline = values_baseline.causal[synram];
					auto& samples = sample_batches.at(batch_index);

					// get samples via neuron mapping from incoming NeuronView
					size_t i = 0;
					for (auto const& column : columns) {
						samples.at(i) = Int8(
						    static_cast<intmax_t>(synram_values[column].value()) -
						    static_cast<intmax_t>(synram_values_baseline[column].value()));
						i++;
					}
				}
			} else {
				for (size_t batch_index = 0; batch_index < m_batch_entries.size(); ++batch_index) {
					assert(m_batch_entries.at(batch_index).m_cadc_values);
					auto const& values = *(m_batch_entries.at(batch_index).m_cadc_values);
					auto const& synram_values = values.causal[synram];
					auto& samples = sample_batches.at(batch_index);

					// get samples via neuron mapping from incoming NeuronView
					size_t i = 0;
					for (auto const& column : columns) {
						samples.at(i) =
						    Int8(static_cast<intmax_t>(synram_values[column].value()) - 128);
						i++;
					}
				}
			}
		}
	}
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::ExternalInput const& data)
{
	if (data.output().type == ConnectionType::DataUInt32) {
		m_local_external_data.uint32.insert({vertex, m_input_list.uint32.at(vertex)});
	} else if (data.output().type == ConnectionType::DataUInt5) {
		m_local_external_data.uint5.insert({vertex, m_input_list.uint5.at(vertex)});
	} else if (data.output().type == ConnectionType::DataInt8) {
		m_local_external_data.int8.insert({vertex, m_input_list.int8.at(vertex)});
	} else if (data.output().type == ConnectionType::DataTimedSpikeSequence) {
		m_local_external_data.spike_events.insert({vertex, m_input_list.spike_events.at(vertex)});
	} else {
		throw std::runtime_error("ExternalInput output type processing not implemented.");
	}
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
			    m_local_data.int8.at(boost::source(in_edge, m_graph.get_graph())).at(j);
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
	m_local_data.int8[vertex] = values;
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
		// We compare with <= and not < and therefore chose the last highest value.
		size_t i = 0;
		for (auto const& entry : local_data) {
			uint32_t index = 0;
			auto max = entry.front();
			uint32_t j = 0;
			for (auto const e : entry) {
				if (max <= e) {
					max = e;
					index = j;
				}
				j++;
			}
			tmps.at(i).at(0) = UInt32(index);
			i++;
		}
		m_local_data.uint32[vertex] = tmps;
	};

	if (data.inputs().front().type == ConnectionType::UInt32) {
		auto const& local_data =
		    m_local_data.uint32.at(boost::source(in_edge, m_graph.get_graph()));
		compute(local_data);
	} else if (data.inputs().front().type == ConnectionType::UInt5) {
		auto const& local_data = m_local_data.uint5.at(boost::source(in_edge, m_graph.get_graph()));
		compute(local_data);
	} else if (data.inputs().front().type == ConnectionType::Int8) {
		auto const& local_data = m_local_data.int8.at(boost::source(in_edge, m_graph.get_graph()));
		compute(local_data);
	} else {
		throw std::logic_error("ArgMax data type not implemented.");
	}
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
		for (size_t i = 0; i < data.output().size; ++i) {
			values.at(j).at(i) = std::max(m_local_data.int8.at(source).at(j).at(i), Int8(0));
		}
	}
	m_local_data.int8[vertex] = values;
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
		for (size_t i = 0; i < data.output().size; ++i) {
			values.at(j).at(i) = UInt5(std::min(
			    static_cast<UInt5::value_type>(
			        std::max(m_local_data.int8.at(source).at(j).at(i), Int8(0)) >> shift),
			    UInt5::max));
		}
	}
	m_local_data.uint5[vertex] = values;
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::DataOutput const& data)
{
	if (data.inputs().front().type == ConnectionType::UInt32) {
		// get in edge
		assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
		auto const in_edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
		auto const& local_data =
		    m_local_data.uint32.at(boost::source(in_edge, m_graph.get_graph()));
		// check size match only for first because we know that the data map is valid
		if (local_data.size()) {
			assert(data.output().size == local_data.front().size());
		}
		m_local_data_output.uint32[vertex] = local_data;
	} else if (data.inputs().front().type == ConnectionType::UInt5) {
		// get in edge
		assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
		auto const in_edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
		auto const& local_data = m_local_data.uint5.at(boost::source(in_edge, m_graph.get_graph()));
		// check size match only for first because we know that the data map is valid
		if (local_data.size()) {
			assert(data.output().size == local_data.front().size());
		}
		m_local_data_output.uint5[vertex] = local_data;
	} else if (data.inputs().front().type == ConnectionType::Int8) {
		// get in edge
		assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
		auto const in_edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
		auto const& local_data = m_local_data.int8.at(boost::source(in_edge, m_graph.get_graph()));
		// check size match only for first because we know that the data map is valid
		assert(!local_data.size() || (data.output().size == local_data.front().size()));
		m_local_data_output.int8[vertex] = local_data;
	} else if (data.inputs().front().type == ConnectionType::DataTimedSpikeFromChipSequence) {
		if (m_event_output_vertex) {
			throw std::logic_error("Only one event output vertex allowed.");
		}
		m_event_output_vertex = vertex;
	} else {
		throw std::logic_error("DataOutput data type not implemented.");
	}
}

void ExecutionInstanceBuilder::register_epilogue(stadls::vx::v2::PlaybackProgramBuilder&& builder)
{
	m_builder_epilogue = std::move(builder);
}

void ExecutionInstanceBuilder::register_prologue(stadls::vx::v2::PlaybackProgramBuilder&& builder)
{
	m_builder_prologue = std::move(builder);
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
	using namespace halco::common;
	using namespace halco::hicann_dls::vx;

	for (auto& batch_entry : m_batch_entries) {
		if (batch_entry.m_ticket) {
			batch_entry.m_cadc_values = batch_entry.m_ticket->get();
		}
		if (batch_entry.m_ticket_baseline) {
			batch_entry.m_cadc_baseline_values = batch_entry.m_ticket_baseline->get();
		}
	}

	if (enable_ppu) {
		for (auto& batch_entry : m_batch_entries) {
			for (auto const ppu : iter_all<PPUOnDLS>()) {
				if (enable_cadc_baseline) {
					if (batch_entry.m_ppu_check_baseline_read[ppu] &&
					    batch_entry.m_ppu_check_baseline_read[ppu]->get().get_value() !=
					        static_cast<uint32_t>(ppu::Status::idle)) {
						throw std::runtime_error("PPU not idle after baseline read.");
					}
				}
				if (batch_entry.m_ppu_check_neuron_reset[ppu] &&
				    batch_entry.m_ppu_check_neuron_reset[ppu]->get().get_value() !=
				        static_cast<uint32_t>(ppu::Status::idle)) {
					throw std::runtime_error("PPU not idle after neuron reset.");
				}
				if (batch_entry.m_ppu_check_read[ppu] &&
				    batch_entry.m_ppu_check_read[ppu]->get().get_value() !=
				        static_cast<uint32_t>(ppu::Status::idle)) {
					throw std::runtime_error("PPU not idle after read.");
				}
			}
		}
	}

	m_postprocessing = true;
	for (auto const vertex : m_post_vertices) {
		std::visit(
		    [&](auto const& value) { process(vertex, value); },
		    m_graph.get_vertex_property(vertex));
	}
	m_postprocessing = false;
	if (m_event_output_vertex) {
		stadls::vx::PlaybackProgram::spikes_type spikes;
		for (auto const& program : m_chunked_program) {
			auto const local_spikes = program.get_spikes();
			spikes.insert(spikes.end(), local_spikes.begin(), local_spikes.end());
		}
		std::sort(spikes.begin(), spikes.end(), [](auto const& a, auto const& b) {
			return a.get_fpga_time() < b.get_fpga_time();
		});
		std::vector<TimedSpikeFromChipSequence> spike_batches;
		auto begin = spikes.begin();
		for (auto const& e : m_batch_entries) {
			assert(e.m_ticket_events_begin);
			auto const begin_time = e.m_ticket_events_begin->get_fpga_time();
			auto const end_time = e.m_ticket_events_end->get_fpga_time();
			begin = std::find_if(begin, spikes.end(), [&](auto const& spike) {
				return spike.get_fpga_time() > begin_time;
			});
			auto const end = std::find_if(begin, spikes.end(), [&](auto const& spike) {
				return spike.get_fpga_time() > end_time;
			});
			spike_batches.push_back(TimedSpikeFromChipSequence(begin, end));
			begin = end;
		}
		m_local_data_output.spike_event_output[*m_event_output_vertex] = spike_batches;
	}
	return std::move(m_local_data_output);
}

std::vector<stadls::vx::v2::PlaybackProgram> ExecutionInstanceBuilder::generate()
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
	using namespace stadls::vx::v2;
	using namespace lola::vx::v2;

	auto [config_builder, ppu_symbols] = m_config_builder.generate(enable_ppu);

	PPUMemoryWordOnPPU ppu_status_coord;
	PPUMemoryBlockOnPPU ppu_result_coord;
	PPUMemoryBlockOnPPU ppu_neuron_reset_mask_coord;
	if (enable_ppu) {
		assert(ppu_symbols);
		ppu_status_coord = ppu_symbols->at("status").coordinate.toMin();
		ppu_result_coord = ppu_symbols->at("cadc_result").coordinate;
		ppu_neuron_reset_mask_coord = ppu_symbols->at("neuron_reset_mask").coordinate;
	}

	std::vector<PlaybackProgramBuilder> builders;
	builders.push_back(std::move(m_builder_prologue));

	PlaybackProgramBuilder builder;

	// if no on-chip computation is to be done, return without static configuration
	auto const has_computation = m_event_input_vertex.has_value();
	if (!has_computation) {
		builder.merge_back(m_builder_epilogue);
		m_chunked_program = {builder.done()};
		return m_chunked_program;
	}

	builder.merge_back(config_builder);

	// timing-uncritical initial setup
	builders.push_back(std::move(builder));
	builder = PlaybackProgramBuilder();

	// build neuron resets
	auto [builder_neuron_reset, _] = stadls::vx::generate(m_neuron_resets);

	if (enable_ppu) {
		for (auto const ppu : iter_all<PPUOnDLS>()) {
			// write neuron reset mask
			std::vector<int8_t> values(NeuronColumnOnDLS::size);
			for (auto const col : iter_all<NeuronColumnOnDLS>()) {
				values[col] =
				    m_neuron_resets.enable_resets[AtomicNeuronOnDLS(col, ppu.toNeuronRowOnDLS())
				                                      .toNeuronResetOnDLS()];
			}
			auto const neuron_reset_mask = to_vector_unit_row(values);
			builder.write(PPUMemoryBlockOnDLS(ppu_neuron_reset_mask_coord, ppu), neuron_reset_mask);
		}
	}

	bool const has_cadc_readout = std::any_of(
	    m_ticket_requests.begin(), m_ticket_requests.end(), [](auto const v) { return v; });

	for (size_t b = 0; b < m_batch_entries.size(); ++b) {
		auto& batch_entry = m_batch_entries.at(b);
		// cadc baseline read
		if (has_cadc_readout && enable_cadc_baseline) {
			if (enable_ppu) {
				// initiate baseline read
				for (auto const ppu : iter_all<PPUOnDLS>()) {
					PPUMemoryWord config(
					    PPUMemoryWord::Value(static_cast<uint32_t>(ppu::Status::baseline_read)));
					builder.write(PPUMemoryWordOnDLS(ppu_status_coord, ppu), config);
				}
				// magic wait for PPU to complete baseline read (Issue #3704)
				builder.write(TimerOnDLS(), Timer());
				builder.block_until(
				    TimerOnDLS(), Timer::Value(100 * Timer::Value::fpga_clock_cycles_per_us));
				for (auto const ppu : iter_all<PPUOnDLS>()) {
					batch_entry.m_ppu_check_baseline_read[ppu] =
					    builder.read(PPUMemoryWordOnDLS(ppu_status_coord, ppu));
				}
			} else {
				// reset neurons (baseline read)
				builder.copy_back(builder_neuron_reset);
				// wait sufficient amount of time (30us) before baseline reads for membrane
				// to settle
				if (!builder.empty()) {
					builder.write(TimerOnDLS(), Timer());
					builder.block_until(
					    TimerOnDLS(), Timer::Value(30 * Timer::Value::fpga_clock_cycles_per_us));
				}
				// readout baselines of neurons
				batch_entry.m_ticket_baseline = builder.read(CADCSamplesOnDLS());
			}
		}
		// reset neurons
		if (enable_ppu) {
			for (auto const ppu : iter_all<PPUOnDLS>()) {
				PPUMemoryWord config(
				    PPUMemoryWord::Value(static_cast<uint32_t>(ppu::Status::reset_neurons)));
				builder.write(PPUMemoryWordOnDLS(ppu_status_coord, ppu), config);
			}
		} else {
			builder.copy_back(builder_neuron_reset);
		}
		// wait for membrane to settle
		if (!builder.empty()) {
			builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
			builder.write(TimerOnDLS(), Timer());
			builder.block_until(
			    TimerOnDLS(),
			    Timer::Value((enable_ppu ? 2 : 1) * Timer::Value::fpga_clock_cycles_per_us));
		}
		if (enable_ppu) {
			for (auto const ppu : iter_all<PPUOnDLS>()) {
				batch_entry.m_ppu_check_neuron_reset[ppu] =
				    builder.read(PPUMemoryWordOnDLS(ppu_status_coord, ppu));
			}
		}
		// send input
		if (m_event_output_vertex) {
			batch_entry.m_ticket_events_begin = builder.read(NullPayloadReadableOnFPGA());
		}
		builder.write(TimerOnDLS(), Timer());
		TimedSpike::Time current_time(0);
		assert(m_event_input_vertex);
		for (auto const& event : m_local_data.spike_events.at(*m_event_input_vertex).at(b)) {
			if (event.time == current_time + 1) {
				current_time = event.time;
			} else if (event.time > current_time) {
				current_time = event.time;
				builder.block_until(TimerOnDLS(), current_time);
			}
			std::visit(
			    [&](auto const& p) {
				    typedef hate::remove_all_qualifiers_t<decltype(p)> container_type;
				    builder.write(typename container_type::coordinate_type(), p);
			    },
			    event.payload);
		}
		if (m_event_output_vertex) {
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
			if (enable_ppu) {
				for (auto const ppu : iter_all<PPUOnDLS>()) {
					PPUMemoryWord config(
					    PPUMemoryWord::Value(static_cast<uint32_t>(ppu::Status::read)));
					builder.write(PPUMemoryWordOnDLS(ppu_status_coord, ppu), config);
				}
				// magic wait for PPU to readout membrane and process read (Issue #3704)
				if (!builder.empty()) {
					builder.write(halco::hicann_dls::vx::TimerOnDLS(), haldls::vx::Timer());
					builder.block_until(
					    halco::hicann_dls::vx::TimerOnDLS(),
					    haldls::vx::Timer::Value(
					        100 * haldls::vx::Timer::Value::fpga_clock_cycles_per_us));
				}
				if (enable_ppu) {
					for (auto const ppu : iter_all<PPUOnDLS>()) {
						batch_entry.m_ppu_check_read[ppu] =
						    builder.read(PPUMemoryWordOnDLS(ppu_status_coord, ppu));
					}
				}
				// readout result
				for (auto const ppu : iter_all<PPUOnDLS>()) {
					batch_entry.m_ppu_result[ppu] =
					    builder.read(PPUMemoryBlockOnDLS(ppu_result_coord, ppu));
				}
			} else {
				batch_entry.m_ticket = builder.read(CADCSamplesOnDLS());
			}
		}
		// wait for response data
		if (!builder.empty()) {
			builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
		}
		builders.push_back(std::move(builder));
		builder = PlaybackProgramBuilder();
	}

	builders.push_back(std::move(m_builder_epilogue));

	// Merge builders sequentially into chunks smaller than the FPGA playback memory size.
	// If a single builder is larger than the memory, it is placed isolated in a program.
	assert(builder.empty());
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
