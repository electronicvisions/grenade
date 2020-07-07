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
#include "grenade/vx/types.h"
#include "haldls/vx/v2/barrier.h"
#include "haldls/vx/v2/padi.h"
#include "hate/timer.h"
#include "hate/type_traits.h"

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
    m_config(chip_config),
    m_post_vertices(),
    m_local_data(),
    m_local_data_output()
{
	// check that input list provides all requested input for local graph
	if (!has_complete_input_list()) {
		throw std::runtime_error("Graph requests unprovided input.");
	}

	m_ticket_requests.fill(false);

	/** Silence everything which is not set in the graph. */
	for (auto& node : m_config.crossbar_nodes) {
		node = haldls::vx::v2::CrossbarNode::drop_all;
	}

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
			if (input_vertex.output().type == ConnectionType::DataOutputInt8) {
				if (m_input_list.int8.find(vertex) == m_input_list.int8.end()) {
					return true;
				} else if (
				    (batch_size != 0) &&
				    m_input_list.int8.at(vertex).front().size() != input_vertex.output().size) {
					return true;
				}
			} else if (input_vertex.output().type == ConnectionType::DataInputUInt16) {
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

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::SynapseArrayView const& data)
{
	auto& config = m_config;
	auto const& columns = data.get_columns();
	auto const synram = data.get_synram();
	auto const hemisphere = synram.toHemisphereOnDLS();
	for (auto const& var : boost::combine(data.get_weights(), data.get_labels(), data.get_rows())) {
		auto const& weights = boost::get<0>(var);
		auto const& labels = boost::get<1>(var);
		auto const& row = boost::get<2>(var);
		auto& weight_row = config.hemispheres[hemisphere].synapse_matrix.weights[row];
		auto& label_row = config.hemispheres[hemisphere].synapse_matrix.labels[row];
		for (size_t index = 0; index < columns.size(); ++index) {
			auto const syn_column = columns[index];
			weight_row[syn_column] = weights[index];
			label_row[syn_column] = labels[index];
		}
	}
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::SynapseDriver const& data)
{
	auto& synapse_driver_config =
	    m_config.hemispheres[data.get_coordinate().toSynapseDriverBlockOnDLS().toHemisphereOnDLS()]
	        .synapse_driver_block[data.get_coordinate().toSynapseDriverOnSynapseDriverBlock()];
	synapse_driver_config.set_row_mode_top(
	    data.get_row_modes()[halco::hicann_dls::vx::v2::SynapseRowOnSynapseDriver::top]);
	synapse_driver_config.set_row_mode_bottom(
	    data.get_row_modes()[halco::hicann_dls::vx::v2::SynapseRowOnSynapseDriver::bottom]);
	synapse_driver_config.set_row_address_compare_mask(data.get_config());
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::PADIBus const& data)
{
	auto const bus = data.get_coordinate();
	auto& config =
	    m_config.hemispheres[bus.toPADIBusBlockOnDLS().toHemisphereOnDLS()].common_padi_bus_config;
	auto enable_config = config.get_enable_spl1();
	enable_config[bus.toPADIBusOnPADIBusBlock()] = true;
	config.set_enable_spl1(enable_config);
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::CrossbarNode const& data)
{
	m_config.crossbar_nodes[data.get_coordinate()] = data.get_config();
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::CrossbarL2Output const&)
{}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::DataInput const& data)
{
	using namespace lola::vx::v2;
	using namespace haldls::vx::v2;
	using namespace halco::hicann_dls::vx::v2;
	using namespace halco::common;
	if (data.output().type == ConnectionType::Int8) {
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
	} else if (data.output().type == ConnectionType::CrossbarInputLabel) {
		assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
		auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
		auto const in_vertex = boost::source(*(in_edges.first), m_graph.get_graph());

		m_local_data.spike_events[vertex] = m_local_external_data.spike_events.at(in_vertex);
	} else {
		throw std::logic_error("DataInput output connection type processing not implemented.");
	}
}

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
		sample_batches.resize(m_batch_entries.size());
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
				samples.resize(data.output().size);

				// get samples via neuron mapping from incoming NeuronView
				size_t i = 0;
				for (auto const& column : columns) {
					auto const tmp = static_cast<intmax_t>(synram_values[column].value()) -
					                 static_cast<intmax_t>(synram_values_baseline[column].value());
					samples.at(i) = Int8(std::min(std::max(tmp, intmax_t(-128)), intmax_t(127)));
					i++;
				}
			}
		} else {
			for (size_t batch_index = 0; batch_index < m_batch_entries.size(); ++batch_index) {
				assert(m_batch_entries.at(batch_index).m_cadc_values);
				auto const& values = *(m_batch_entries.at(batch_index).m_cadc_values);
				auto const& synram_values = values.causal[synram];
				auto& samples = sample_batches.at(batch_index);
				samples.resize(data.output().size);

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
	// TODO: once we have neuron configuration, it should be placed here
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /*vertex*/, vertex::NeuronEventOutputView const& /* data */)
{}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::ExternalInput const& data)
{
	if (data.output().type == ConnectionType::DataOutputInt8) {
		m_local_external_data.int8.insert({vertex, m_input_list.int8.at(vertex)});
	} else if (data.output().type == ConnectionType::DataInputUInt16) {
		m_local_external_data.spike_events.insert({vertex, m_input_list.spike_events.at(vertex)});
	} else {
		throw std::runtime_error("ExternalInput output type processing not implemented.");
	}
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::Addition const& data)
{
	auto const batch_size = m_input_list.batch_size();
	std::vector<std::vector<Int8>> values(batch_size);
	for (auto& entry : values) {
		entry.resize(data.output().size);
	}

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
    Graph::vertex_descriptor const vertex, vertex::DataOutput const& data)
{
	if (data.inputs().front().type == ConnectionType::Int8) {
		// get in edge
		assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
		auto const in_edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
		auto const& local_data = m_local_data.int8.at(boost::source(in_edge, m_graph.get_graph()));
		// check size match only for first because we know that the data map is valid
		assert(!local_data.size() || (data.output().size == local_data.front().size()));
		m_local_data_output.int8[vertex] = local_data;
	} else if (data.inputs().front().type == ConnectionType::DataOutputUInt16) {
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
}

IODataMap ExecutionInstanceBuilder::post_process()
{
	for (auto& batch_entry : m_batch_entries) {
		if (batch_entry.m_ticket) {
			batch_entry.m_cadc_values = batch_entry.m_ticket->get();
		}
		if (batch_entry.m_ticket_baseline) {
			batch_entry.m_cadc_baseline_values = batch_entry.m_ticket_baseline->get();
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
		auto spikes = m_program.get_spikes();
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

stadls::vx::v2::PlaybackProgram ExecutionInstanceBuilder::generate()
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
	using namespace stadls::vx::v2;
	using namespace lola::vx::v2;

	PlaybackProgramBuilder builder = std::move(m_builder_prologue);

	// generate input event sequence
	if (m_local_data.spike_events.size() > 1) {
		throw std::logic_error("Expected only one spike input vertex.");
	}
	bool const has_computation = m_local_data.spike_events.size() == 1;
	if (!has_computation) {
		builder.merge_back(m_builder_epilogue);
		m_program = builder.done();
		return m_program;
	}

	// write static configuration
	for (auto const hemisphere : iter_all<HemisphereOnDLS>()) {
		builder.write(SynramOnDLS(hemisphere), m_config.hemispheres[hemisphere].synapse_matrix);
		for (auto const synapse_driver : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
			builder.write(
			    SynapseDriverOnDLS(synapse_driver, SynapseDriverBlockOnDLS(hemisphere)),
			    m_config.hemispheres[hemisphere].synapse_driver_block[synapse_driver]);
		}
		builder.write(
		    hemisphere.toCommonPADIBusConfigOnDLS(),
		    m_config.hemispheres[hemisphere].common_padi_bus_config);
	}
	for (auto const coord : iter_all<CrossbarNodeOnDLS>()) {
		builder.write(coord, m_config.crossbar_nodes[coord]);
	}

	// build neuron resets
	auto [builder_neuron_reset, _] = stadls::vx::generate(m_neuron_resets);

	bool const has_cadc_readout = std::any_of(
	    m_ticket_requests.begin(), m_ticket_requests.end(), [](auto const v) { return v; });

	for (size_t b = 0; b < m_batch_entries.size(); ++b) {
		auto& batch_entry = m_batch_entries.at(b);

		// cadc baseline read
		if (has_cadc_readout && enable_cadc_baseline) {
			// reset neurons (baseline read)
			builder.copy_back(builder_neuron_reset);
			// wait sufficient amount of time (30us) before baseline reads for membrane
			// to settle
			if (!builder.empty()) {
				builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
				builder.write(halco::hicann_dls::vx::TimerOnDLS(), haldls::vx::Timer());
				builder.block_until(
				    halco::hicann_dls::vx::TimerOnDLS(),
				    haldls::vx::Timer::Value(
				        30 * haldls::vx::Timer::Value::fpga_clock_cycles_per_us));
			}
			// readout baselines of neurons
			batch_entry.m_ticket_baseline = builder.read(CADCSamplesOnDLS());
		}

		// reset neurons
		builder.copy_back(builder_neuron_reset);
		builder.block_until(BarrierOnFPGA(), Barrier::omnibus);

		// wait for membrane to settle
		if (!builder.empty()) {
			builder.write(halco::hicann_dls::vx::TimerOnDLS(), haldls::vx::Timer());
			builder.block_until(
			    halco::hicann_dls::vx::TimerOnDLS(),
			    haldls::vx::Timer::Value::fpga_clock_cycles_per_us);
		}

		// send input
		if (m_event_output_vertex) {
			batch_entry.m_ticket_events_begin = builder.read(NullPayloadReadableOnFPGA());
		}
		builder.write(TimerOnDLS(), Timer());
		TimedSpike::Time current_time(0);
		for (auto const& event : m_local_data.spike_events.begin()->second.at(b)) {
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
			        2 * haldls::vx::Timer::Value::fpga_clock_cycles_per_us));
		}

		// read out neuron membranes
		if (has_cadc_readout) {
			batch_entry.m_ticket = builder.read(CADCSamplesOnDLS());
		}
	}

	// wait for response data
	if (!builder.empty()) {
		builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
	}

	builder.merge_back(m_builder_epilogue);
	m_program = builder.done();
	return m_program;
}

} // namespace grenade::vx
