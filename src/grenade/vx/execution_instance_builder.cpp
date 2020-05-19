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
#include "haldls/vx/padi.h"
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
    DataMap const& input_list,
    DataMap const& data_output,
    ChipConfig const& chip_config) :
    m_graph(graph),
    m_input_list(input_list),
    m_data_output(data_output),
    m_local_external_data(),
    m_config(),
    m_builder_input(),
    m_builder_neuron_reset(),
    m_post_vertices(),
    m_local_data(),
    m_local_data_output(),
    m_ticket()
{
	*m_config = chip_config;

	m_neuron_resets.fill(false);
	m_ticket_requests.fill(false);
	m_used_padi_busses.fill(false);

	/** Silence everything which is not set in the graph. */
	for (auto& node : m_config->crossbar_nodes) {
		node = haldls::vx::CrossbarNode::drop_all;
	}

	m_postprocessing = false;
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
	// set synaptic weights and addresses
	if (enable_hagen_workarounds) { // TODO: remove once HAGEN-mode bug is resolved
		for (auto const& var :
		     boost::combine(data.get_weights(), data.get_labels(), data.get_rows())) {
			auto const& weights = boost::get<0>(var);
			[[maybe_unused]] auto const& labels = boost::get<1>(var);
			auto const& row = boost::get<2>(var);
			auto& weight_row = config.hemispheres[hemisphere].synapse_matrix.weights[row];
			auto& label_row = config.hemispheres[hemisphere].synapse_matrix.labels[row];
			for (size_t index = 0; index < columns.size(); ++index) {
				auto const syn_column = columns[index];
				weight_row[syn_column] = weights.at(index);
				label_row[syn_column] =
				    m_hagen_addresses[halco::hicann_dls::vx::SynapseRowOnDLS(row, synram)];
			}
		}
	} else {
		for (auto const& var :
		     boost::combine(data.get_weights(), data.get_labels(), data.get_rows())) {
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
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::SynapseDriver const& data)
{
	auto& synapse_driver_config =
	    m_config->hemispheres[data.get_coordinate().toSynapseDriverBlockOnDLS().toHemisphereOnDLS()]
	        .synapse_driver_block[data.get_coordinate().toSynapseDriverOnSynapseDriverBlock()];
	synapse_driver_config.set_row_mode_top(
	    data.get_row_modes()[halco::hicann_dls::vx::SynapseRowOnSynapseDriver::top]);
	synapse_driver_config.set_row_mode_bottom(
	    data.get_row_modes()[halco::hicann_dls::vx::SynapseRowOnSynapseDriver::bottom]);
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
    Graph::vertex_descriptor const vertex, vertex::DataInput const& data)
{
	using namespace lola::vx;
	using namespace haldls::vx;
	using namespace halco::hicann_dls::vx;
	using namespace halco::common;
	auto const& vertex_map = m_graph.get_vertex_property_map();
	if (data.output().type == ConnectionType::Int8) {
		assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
		auto const edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
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

		if (enable_hagen_workarounds) { // TODO: remove once HAGEN-mode bug is resolved
			auto const out_edges = boost::out_edges(vertex, m_graph.get_graph());

			auto const process_label = [&](vertex::CrossbarNode const& crossbar_node,
			                               vertex::SynapseDriver const& synapse_driver,
			                               vertex::SynapseArrayView const& synapse_array,
			                               haldls::vx::SpikeLabel const& label) {
				auto const spl1_address =
				    crossbar_node.get_coordinate().toCrossbarInputOnDLS().toSPL1Address();
				if (!spl1_address) { // no spl1 channel
					return;
				}
				if (*spl1_address != label.get_spl1_address()) { // wrong spl1 channel
					return;
				}
				auto const neuron_label = label.get_neuron_label();
				auto const target = crossbar_node.get_config().get_target();
				auto const mask = crossbar_node.get_config().get_mask();
				if ((neuron_label & mask) != target) { // crossbar node drops event
					return;
				}
				auto const syndrv_mask = synapse_driver.get_config().value();
				if ((synapse_driver.get_coordinate()
				         .toSynapseDriverOnSynapseDriverBlock()
				         .toSynapseDriverOnPADIBus() &
				     syndrv_mask) !=
				    (label.get_row_select_address() & syndrv_mask)) { // synapse driver drops event
					return;
				}
				auto const synapse_rows = synapse_driver.get_coordinate().toSynapseRowOnSynram();
				auto const synapse_label = label.get_synapse_label();
				// FIXME: add all two allowed values per row (i.e. two)
				for (auto const& row : synapse_rows) {
					auto const it = std::find(
					    synapse_array.get_rows().begin(), synapse_array.get_rows().end(), row);
					if (it != synapse_array.get_rows().end()) {
						size_t const i = std::distance(synapse_array.get_rows().begin(), it);
						auto const& label_row = synapse_array.get_labels()[i];
						if ((label_row.at(0) & (1 << 5)) ==
						    (synapse_label & (1 << 5))) { // right hagen address
							m_hagen_addresses[SynapseRowOnDLS(row, synapse_array.get_synram())] =
							    synapse_label;
						}
					}
				}
			};

			auto const process_events = [&](vertex::CrossbarNode const& crossbar_node,
			                                vertex::SynapseDriver const& synapse_driver,
			                                vertex::SynapseArrayView const& synapse_array) {
				auto const& events = m_local_data.spike_events.at(vertex);
				for (auto const& event : events) {
					std::visit(
					    [&](auto const& e) {
						    for (auto const& label : e.get_labels()) {
							    process_label(crossbar_node, synapse_driver, synapse_array, label);
						    }
					    },
					    event.payload);
				}
			};

			auto const process_crossbar_node = [&](auto const& out_edge) {
				auto const crossbar_node_vertex = boost::target(out_edge, m_graph.get_graph());
				auto const& crossbar_node =
				    std::get<vertex::CrossbarNode>(vertex_map.at(crossbar_node_vertex));
				auto const crossbar_node_out_edges =
				    boost::out_edges(crossbar_node_vertex, m_graph.get_graph());
				for (auto const crossbar_node_out_edge :
				     boost::make_iterator_range(crossbar_node_out_edges)) {
					auto const crossbar_node_target_vertex =
					    boost::target(crossbar_node_out_edge, m_graph.get_graph());
					if (std::holds_alternative<vertex::PADIBus>(
					        vertex_map.at(crossbar_node_target_vertex))) {
						auto const padi_bus_out_edges =
						    boost::out_edges(crossbar_node_target_vertex, m_graph.get_graph());
						for (auto const padi_bus_out_edge :
						     boost::make_iterator_range(padi_bus_out_edges)) {
							auto const synapse_driver_vertex =
							    boost::target(padi_bus_out_edge, m_graph.get_graph());
							auto const& synapse_driver = std::get<vertex::SynapseDriver>(
							    vertex_map.at(synapse_driver_vertex));
							auto const syndrv_out_edges =
							    boost::out_edges(synapse_driver_vertex, m_graph.get_graph());
							for (auto const syndrv_out_edge :
							     boost::make_iterator_range(syndrv_out_edges)) {
								auto const synapse_array_vertex =
								    boost::target(syndrv_out_edge, m_graph.get_graph());
								auto const& synapse_array = std::get<vertex::SynapseArrayView>(
								    vertex_map.at(synapse_array_vertex));
								process_events(crossbar_node, synapse_driver, synapse_array);
							}
						}
					}
				}
			};
			for (auto const out_edge : boost::make_iterator_range(out_edges)) {
				process_crossbar_node(out_edge);
			}
		}
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
	using namespace halco::hicann_dls::vx;
	using namespace lola::vx;
	using namespace haldls::vx;
	if (!m_postprocessing) { // pre-hw-run processing
		m_ticket_requests[hemisphere] = true;
		// results need hardware execution
		m_post_vertices.push_back(vertex);
	} else { // post-hw-run processing
		// extract Int8 values
		assert(m_ticket);
		auto const values = m_ticket->get();
		assert(m_ticket_baseline);
		auto const values_baseline = m_ticket_baseline->get();
		std::vector<Int8> samples(data.output().size);

		// get samples via neuron mapping from incoming NeuronView
		size_t i = 0;
		for (auto const& column : data.get_columns()) {
			samples.at(i) = Int8(
			    static_cast<intmax_t>(values.causal[data.get_synram()][column].value()) -
			    static_cast<intmax_t>(values_baseline.causal[data.get_synram()][column].value()));
			i++;
		}
		m_local_data.int8[vertex] = samples;
	}
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /*vertex*/, vertex::NeuronView const& data)
{
	using namespace halco::hicann_dls::vx;
	using namespace haldls::vx;
	// TODO: This reset does not really belong here / how do we say when and whether it should
	// be triggered?
	// result: -> make property of each neuron view entry
	for (auto const column : data.get_columns()) {
		auto const neuron_reset = AtomicNeuronOnDLS(column, data.get_row()).toNeuronResetOnDLS();
		m_neuron_resets[neuron_reset] = true;
	}
	// TODO: once we have neuron configuration, it should be placed here
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::ExternalInput const& data)
{
	if (data.output().type == ConnectionType::DataOutputInt8) {
		m_local_external_data.int8[vertex] = m_input_list.int8.at(vertex);
	} else if (data.output().type == ConnectionType::DataOutputUInt16) {
		m_local_external_data.spike_events[vertex] = m_input_list.spike_events.at(vertex);
	} else {
		throw std::runtime_error("ExternalInput output type processing not implemented.");
	}
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::Addition const& data)
{
	std::vector<Int8> values(data.output().size);
	std::fill(values.begin(), values.end(), Int8(0));
	auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
	for (size_t i = 0; i < data.output().size; ++i) {
		intmax_t tmp = 0;
		for (auto const in_edge : boost::make_iterator_range(in_edges)) {
			tmp += m_local_data.int8.at(boost::source(in_edge, m_graph.get_graph())).at(i);
		}
		// restrict to range [-128,127]
		values.at(i) = Int8(std::min(std::max(tmp, intmax_t(-128)), intmax_t(127)));
	}
	m_local_data.int8[vertex] = values;
}

template <>
void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::DataOutput const& data)
{
	if (data.inputs().front().type == ConnectionType::Int8) {
		std::vector<Int8> values(data.output().size);
		// get mapping
		auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
		for (auto const in_edge : boost::make_iterator_range(in_edges)) {
			for (size_t i = 0; i < values.size(); ++i) {
				// perform lookup
				values.at(i) =
				    m_local_data.int8.at(boost::source(in_edge, m_graph.get_graph())).at(i);
			}
		}
		m_local_data_output.int8[vertex] = values;
	} else {
		throw std::logic_error("DataOutput data type not implemented.");
	}
}

void ExecutionInstanceBuilder::process(Graph::vertex_descriptor const vertex)
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceBuilder");
	if (inputs_available(vertex)) {
		auto const& vertex_map = m_graph.get_vertex_property_map();
		std::visit(
		    [&](auto const& value) {
			    hate::Timer timer;
			    process(vertex, value);
			    LOG4CXX_TRACE(
			        logger, "process(): Preprocessed "
			                    << name<hate::remove_all_qualifiers_t<decltype(value)>>() << " in "
			                    << timer.print() << ".");
		    },
		    vertex_map.at(vertex));
	} else {
		m_post_vertices.push_back(vertex);
	}
}

void ExecutionInstanceBuilder::post_process(Graph::vertex_descriptor const vertex)
{
	auto const& vertex_map = m_graph.get_vertex_property_map();
	std::visit([&](auto const& value) { process(vertex, value); }, vertex_map.at(vertex));
}

DataMap ExecutionInstanceBuilder::post_process()
{
	m_postprocessing = true;
	for (auto const vertex : m_post_vertices) {
		post_process(vertex);
	}
	m_postprocessing = false;
	m_ticket.reset();
	m_ticket_baseline.reset();
	m_post_vertices.clear();
	m_local_external_data.clear();
	m_local_data.clear();
	return std::move(m_local_data_output);
}

stadls::vx::PlaybackProgramBuilder ExecutionInstanceBuilder::generate()
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx;
	using namespace haldls::vx;
	using namespace stadls::vx;
	using namespace lola::vx;

	// generate input event sequence
	if (m_local_data.spike_events.size() > 1) {
		throw std::logic_error("Expected only one spike input vertex.");
	}
	if (m_local_data.spike_events.size() > 0) {
		m_builder_input.write(TimerOnDLS(), Timer());
		TimedSpikeEvent::Time current_time(0);
		for (auto const& event : m_local_data.spike_events.begin()->second) {
			if (event.time > current_time) {
				current_time = event.time;
				m_builder_input.wait_until(TimerOnDLS(), current_time);
			}
			std::visit(
			    [&](auto const& p) {
				    typedef hate::remove_all_qualifiers_t<decltype(p)> container_type;
				    m_builder_input.write(typename container_type::coordinate_type(), p);
			    },
			    event.payload);
		}
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
		m_ticket_baseline = m_builder_cadc_readout_baseline.read(CADCSamplesOnDLS());
		m_ticket = m_builder_cadc_readout.read(CADCSamplesOnDLS());
	}
	m_ticket_requests.fill(false);

	PlaybackProgramBuilder builder;
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
		auto const new_matrix = std::make_unique<lola::vx::SynapseMatrix>();
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
			    haldls::vx::SynapseDriverConfig::RowMode::disabled);
			m_config->hemispheres[hemisphere].synapse_driver_block[drv].set_row_mode_bottom(
			    haldls::vx::SynapseDriverConfig::RowMode::disabled);
		}
	}
	for (auto& a : m_hagen_addresses) {
		a = lola::vx::SynapseMatrix::Label(0);
	}
	m_used_padi_busses.fill(false);
	// build neuron resets
	for (auto const neuron : iter_all<NeuronResetOnDLS>()) {
		if (m_neuron_resets[neuron]) {
			m_builder_neuron_reset.write(neuron, NeuronReset());
		}
	}
	m_neuron_resets.fill(false);
	// reset neurons (baseline read)
	builder.copy_back(m_builder_neuron_reset);
	// wait sufficient amount of time (30us) before baseline reads for membrane to settle
	if (!builder.empty()) {
		builder.write(halco::hicann_dls::vx::TimerOnDLS(), haldls::vx::Timer());
		builder.wait_until(
		    halco::hicann_dls::vx::TimerOnDLS(),
		    haldls::vx::Timer::Value(30 * haldls::vx::Timer::Value::fpga_clock_cycles_per_us));
	}
	// readout baselines of neurons
	builder.merge_back(m_builder_cadc_readout_baseline);
	// reset neurons
	builder.merge_back(m_builder_neuron_reset);
	// send input
	builder.merge_back(m_builder_input);
	// wait for membrane to settle
	if (!builder.empty()) {
		builder.write(halco::hicann_dls::vx::TimerOnDLS(), haldls::vx::Timer());
		builder.wait_until(
		    halco::hicann_dls::vx::TimerOnDLS(),
		    haldls::vx::Timer::Value(2 * haldls::vx::Timer::Value::fpga_clock_cycles_per_us));
	}
	// read out neuron membranes
	builder.merge_back(m_builder_cadc_readout);
	// wait for response data
	if (!builder.empty()) {
		builder.write(halco::hicann_dls::vx::TimerOnDLS(), haldls::vx::Timer());
		builder.wait_until(halco::hicann_dls::vx::TimerOnDLS(), haldls::vx::Timer::Value(100000));
	}

	return builder;
}

} // namespace grenade::vx
