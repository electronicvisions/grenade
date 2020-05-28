#include "grenade/vx/execution_instance_builder.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <boost/range/combine.hpp>
#include <boost/type_index.hpp>
#include <log4cxx/logger.h>
#include <tbb/parallel_for_each.h>

#include "grenade/vx/data_map.h"
#include "grenade/vx/execution_instance.h"
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
    DataMap const& input_list,
    DataMap const& data_output,
    ChipConfig const& chip_config) :
    m_graph(graph),
    m_execution_instance(execution_instance),
    m_input_list(input_list),
    m_data_output(data_output),
    m_local_external_data(),
    m_config(),
    m_post_vertices(),
    m_local_data(),
    m_local_data_output()
{
	// check that input list provides all requested input for local graph
	if (!has_complete_input_list()) {
		throw std::runtime_error("Graph requests unprovided input.");
	}

	*m_config = chip_config;

	m_ticket_requests.fill(false);
	m_used_padi_busses.fill(false);

	/** Silence everything which is not set in the graph. */
	for (auto& node : m_config->crossbar_nodes) {
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
	auto const& vertex_property_map = m_graph.get_vertex_property_map();
	auto const execution_instance_vertex =
	    m_graph.get_execution_instance_map().right.at(m_execution_instance);
	auto const vertices = boost::make_iterator_range(
	    m_graph.get_vertex_descriptor_map().right.equal_range(execution_instance_vertex));
	return std::none_of(vertices.begin(), vertices.end(), [&](auto const& p) {
		auto const vertex = p.second;
		if (std::holds_alternative<vertex::ExternalInput>(vertex_property_map.at(vertex))) {
			auto const& input_vertex =
			    std::get<vertex::ExternalInput>(vertex_property_map.at(vertex));
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
	auto& config = *m_config;
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
			weight_row[syn_column] = weights.at(index);
			label_row[syn_column] = labels.at(index);
		}
	}
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::SynapseDriver const& data)
{
	auto& synapse_driver_config =
	    m_config->hemispheres[data.get_coordinate().toSynapseDriverBlockOnDLS().toHemisphereOnDLS()]
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
	m_used_padi_busses[data.get_coordinate()] = true;
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::CrossbarNode const& data)
{
	m_config->crossbar_nodes[data.get_coordinate()] = data.get_config();
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
	auto const& vertex_map = m_graph.get_vertex_property_map();
	if (data.output().type == ConnectionType::Int8) {
		assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
		auto const edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
		if (m_graph.get_edge_property_map().at(edge)) {
			throw std::logic_error("Edge with port restriction unsupported.");
		}
		auto const in_vertex = boost::source(edge, m_graph.get_graph());
		auto const& input_values =
		    ((std::holds_alternative<vertex::ExternalInput>(vertex_map.at(in_vertex)))
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
	auto const hemisphere = data.get_synram().toHemisphereOnDLS();

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
		for (size_t batch_index = 0; batch_index < m_batch_entries.size(); ++batch_index) {
			assert(m_batch_entries.at(batch_index).m_ticket);
			auto const values = m_batch_entries.at(batch_index).m_ticket->get();
			assert(m_batch_entries.at(batch_index).m_ticket_baseline);
			auto const values_baseline = m_batch_entries.at(batch_index).m_ticket_baseline->get();
			std::vector<Int8> samples(data.output().size);

			// get samples via neuron mapping from incoming NeuronView
			size_t i = 0;
			for (auto const& column : data.get_columns()) {
				auto const tmp =
				    static_cast<intmax_t>(values.causal[data.get_synram()][column].value()) -
				    static_cast<intmax_t>(
				        values_baseline.causal[data.get_synram()][column].value());
				samples.at(i) = Int8(std::min(std::max(tmp, intmax_t(-128)), intmax_t(127)));
				i++;
			}
			sample_batches.push_back(samples);
		}
		m_local_data.int8[vertex] = sample_batches;
	}
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /*vertex*/, vertex::NeuronView const& data)
{
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
	// TODO: This reset does not really belong here / how do we say when and whether it should
	// be triggered?
	// result: -> make property of each neuron view entry
	for (auto const column : data.get_columns()) {
		auto const neuron_reset = AtomicNeuronOnDLS(column, data.get_row()).toNeuronResetOnDLS();
		m_neuron_resets.enable_resets[neuron_reset] = true;
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
	std::vector<std::vector<Int8>> values(m_input_list.batch_size());
	for (auto& entry : values) {
		entry.resize(data.output().size);
	}
	auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
	for (size_t j = 0; j < values.size(); ++j) {
		for (size_t i = 0; i < data.output().size; ++i) {
			intmax_t tmp = 0;
			for (auto const in_edge : boost::make_iterator_range(in_edges)) {
				tmp +=
				    m_local_data.int8.at(boost::source(in_edge, m_graph.get_graph())).at(j).at(i);
			}
			// restrict to range [-128,127]
			values.at(j).at(i) = Int8(std::min(std::max(tmp, intmax_t(-128)), intmax_t(127)));
		}
	}
	m_local_data.int8[vertex] = values;
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::DataOutput const& data)
{
	if (data.inputs().front().type == ConnectionType::Int8) {
		std::vector<std::vector<Int8>> values(m_input_list.batch_size());
		for (auto& entry : values) {
			entry.resize(data.output().size);
		}
		// get mapping
		auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
		for (auto const in_edge : boost::make_iterator_range(in_edges)) {
			for (size_t j = 0; j < values.size(); ++j) {
				for (size_t i = 0; i < data.output().size; ++i) {
					// perform lookup
					values.at(j).at(i) =
					    m_local_data.int8.at(boost::source(in_edge, m_graph.get_graph()))
					        .at(j)
					        .at(i);
				}
			}
		}
		m_local_data_output.int8[vertex] = values;
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
			auto const& vertex_map = m_graph.get_vertex_property_map();
			std::visit(
			    [&](auto const& value) {
				    hate::Timer timer;
				    process(vertex, value);
				    LOG4CXX_TRACE(
				        logger, "process(): Preprocessed "
				                    << name<hate::remove_all_qualifiers_t<decltype(value)>>()
				                    << " in " << timer.print() << ".");
			    },
			    vertex_map.at(vertex));
		} else {
			m_post_vertices.push_back(vertex);
		}
	}
}

DataMap ExecutionInstanceBuilder::post_process()
{
	auto const& vertex_map = m_graph.get_vertex_property_map();
	m_postprocessing = true;
	for (auto const vertex : m_post_vertices) {
		std::visit([&](auto const& value) { process(vertex, value); }, vertex_map.at(vertex));
	}
	m_postprocessing = false;
	for (auto& batch_entry : m_batch_entries) {
		batch_entry.m_ticket.reset();
		batch_entry.m_ticket_baseline.reset();
	}
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
	m_event_output_vertex.reset();
	m_post_vertices.clear();
	m_local_external_data.clear();
	m_local_data.clear();
	return std::move(m_local_data_output);
}

stadls::vx::v2::PlaybackProgram ExecutionInstanceBuilder::generate()
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
	using namespace stadls::vx::v2;
	using namespace lola::vx::v2;

	// generate input event sequence
	std::vector<PlaybackProgramBuilder> builder_input(m_batch_entries.size());
	if (m_local_data.spike_events.size() == 1) {
		for (size_t b = 0; b < builder_input.size(); ++b) {
			m_batch_entries.at(b).m_ticket_events_begin =
			    builder_input.at(b).read(EventRecordingConfigOnFPGA());
			builder_input.at(b).write(TimerOnDLS(), Timer());
			TimedSpike::Time current_time(0);
			for (auto const& event : m_local_data.spike_events.begin()->second.at(b)) {
				if (event.time > current_time) {
					current_time = event.time;
					builder_input.at(b).block_until(TimerOnDLS(), current_time);
				}
				std::visit(
				    [&](auto const& p) {
					    typedef hate::remove_all_qualifiers_t<decltype(p)> container_type;
					    builder_input.at(b).write(typename container_type::coordinate_type(), p);
				    },
				    event.payload);
			}
			m_batch_entries.at(b).m_ticket_events_end =
			    builder_input.at(b).read(EventRecordingConfigOnFPGA());
		}
	} else if (m_local_data.spike_events.size() > 1) {
		throw std::logic_error("Expected only one spike input vertex.");
	}

	// apply padi bus configuration
	for (auto const bus : iter_all<PADIBusOnDLS>()) {
		auto& config = m_config->hemispheres[bus.toPADIBusBlockOnDLS().toHemisphereOnDLS()]
		                   .common_padi_bus_config;
		auto enable_config = m_config->hemispheres[bus.toPADIBusBlockOnDLS().toHemisphereOnDLS()]
		                         .common_padi_bus_config.get_enable_spl1();
		enable_config[bus.toPADIBusOnPADIBusBlock()] = m_used_padi_busses[bus];
		config.set_enable_spl1(enable_config);
	}

	// build cadc readouts
	if (std::any_of(
	        m_ticket_requests.begin(), m_ticket_requests.end(), [](auto const v) { return v; })) {
		for (auto& batch_entry : m_batch_entries) {
			batch_entry.m_ticket_baseline =
			    batch_entry.m_builder_cadc_readout_baseline.read(CADCSamplesOnDLS());
			batch_entry.m_ticket = batch_entry.m_builder_cadc_readout.read(CADCSamplesOnDLS());
		}
	}
	m_ticket_requests.fill(false);

	PlaybackProgramBuilder builder;
	builder.merge_back(m_builder_prologue);

	// write static configuration
	if (!m_config.has_history()) {
		for (auto const& hemisphere : iter_all<HemisphereOnDLS>()) {
			builder.write(
			    SynramOnDLS(hemisphere), m_config->hemispheres[hemisphere].synapse_matrix);
			for (auto const synapse_driver : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
				builder.write(
				    SynapseDriverOnDLS(synapse_driver, SynapseDriverBlockOnDLS(hemisphere)),
				    m_config->hemispheres[hemisphere].synapse_driver_block[synapse_driver]);
			}
			builder.write(
			    hemisphere.toCommonPADIBusConfigOnDLS(),
			    m_config->hemispheres[hemisphere].common_padi_bus_config);
		}
		for (auto const coord : iter_all<CrossbarNodeOnDLS>()) {
			builder.write(coord, m_config->crossbar_nodes[coord]);
		}
	} else {
		for (auto const& hemisphere : iter_all<HemisphereOnDLS>()) {
			builder.write(
			    SynramOnDLS(hemisphere), m_config->hemispheres[hemisphere].synapse_matrix,
			    m_config.get_history().hemispheres[hemisphere].synapse_matrix);
			for (auto const synapse_driver : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
				builder.write(
				    SynapseDriverOnDLS(synapse_driver, SynapseDriverBlockOnDLS(hemisphere)),
				    m_config->hemispheres[hemisphere].synapse_driver_block[synapse_driver],
				    m_config.get_history()
				        .hemispheres[hemisphere]
				        .synapse_driver_block[synapse_driver]);
			}
			builder.write(
			    hemisphere.toCommonPADIBusConfigOnDLS(),
			    m_config->hemispheres[hemisphere].common_padi_bus_config);
		}
		for (auto const coord : iter_all<CrossbarNodeOnDLS>()) {
			builder.write(coord, m_config->crossbar_nodes[coord]);
		}
	}
	m_config.save();
	// reset
	{
		auto const new_matrix = std::make_unique<lola::vx::v2::SynapseMatrix>();
		for (auto const& hemisphere : iter_all<HemisphereOnDLS>()) {
			m_config->hemispheres[hemisphere].synapse_matrix = *new_matrix;
		}
	}
	for (auto& node : m_config->crossbar_nodes) {
		node = CrossbarNode::drop_all;
	}
	for (auto const& hemisphere : iter_all<HemisphereOnDLS>()) {
		for (auto const drv : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
			m_config->hemispheres[hemisphere].synapse_driver_block[drv].set_row_mode_top(
			    haldls::vx::v2::SynapseDriverConfig::RowMode::disabled);
			m_config->hemispheres[hemisphere].synapse_driver_block[drv].set_row_mode_bottom(
			    haldls::vx::v2::SynapseDriverConfig::RowMode::disabled);
		}
	}
	m_used_padi_busses.fill(false);
	// build neuron resets
	auto [builder_neuron_reset, _] = stadls::vx::v2::generate(m_neuron_resets);
	// insert batches
	for (size_t b = 0; b < m_batch_entries.size(); ++b) {
		// reset neurons (baseline read)
		builder.copy_back(builder_neuron_reset);
		// wait sufficient amount of time (30us) before baseline reads for membrane to settle
		if (!builder.empty()) {
			builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
			builder.write(halco::hicann_dls::vx::v2::TimerOnDLS(), haldls::vx::v2::Timer());
			builder.block_until(
			    halco::hicann_dls::vx::v2::TimerOnDLS(),
			    haldls::vx::v2::Timer::Value(
			        30 * haldls::vx::v2::Timer::Value::fpga_clock_cycles_per_us));
		}
		// readout baselines of neurons
		builder.merge_back(m_batch_entries.at(b).m_builder_cadc_readout_baseline);
		// reset neurons
		builder.copy_back(builder_neuron_reset);
		builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
		// send input
		builder.merge_back(builder_input.at(b));
		// wait for membrane to settle
		if (!builder.empty()) {
			builder.write(halco::hicann_dls::vx::v2::TimerOnDLS(), haldls::vx::v2::Timer());
			builder.block_until(
			    halco::hicann_dls::vx::v2::TimerOnDLS(),
			    haldls::vx::v2::Timer::Value(
			        2 * haldls::vx::v2::Timer::Value::fpga_clock_cycles_per_us));
		}
		// read out neuron membranes
		builder.merge_back(m_batch_entries.at(b).m_builder_cadc_readout);
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
