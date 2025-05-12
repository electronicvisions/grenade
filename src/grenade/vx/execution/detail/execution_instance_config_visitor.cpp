#include "grenade/vx/execution/detail/execution_instance_config_visitor.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/execution/detail/ppu_program_generator.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/signal_flow/vertex/pad_readout.h"
#include "halco/hicann-dls/vx/v3/readout.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/padi.h"
#include "haldls/vx/v3/readout.h"
#include "hate/timer.h"
#include "hate/type_index.h"
#include "hate/type_traits.h"
#include "hate/variant.h"
#include "lola/vx/ppu.h"
#include <filesystem>
#include <iterator>
#include <vector>
#include <boost/range/combine.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail {

ExecutionInstanceConfigVisitor::ExecutionInstanceConfigVisitor(
    signal_flow::Graph const& graph,
    grenade::common::ExecutionInstanceID const& execution_instance) :
    m_graph(graph), m_execution_instance(execution_instance)
{
}

template <typename T>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    T const& /* data */,
    lola::vx::v3::Chip& /* chip */) const
{
	// Spezialize for types which change static configuration
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::CADCMembraneReadoutView const& data,
    lola::vx::v3::Chip& chip) const
{
	// Configure neurons
	for (size_t i = 0; i < data.get_columns().size(); ++i) {
		for (size_t j = 0; j < data.get_columns().at(i).size(); ++j) {
			halco::hicann_dls::vx::v3::AtomicNeuronOnDLS neuron(
			    data.get_columns().at(i).at(j).toNeuronColumnOnDLS(),
			    data.get_synram().toNeuronRowOnDLS());
			chip.neuron_block.atomic_neurons[neuron].readout.source =
			    data.get_sources().at(i).at(j);
			chip.neuron_block.atomic_neurons[neuron].readout.enable_amplifier = true;
		}
	}
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const,
    signal_flow::vertex::NeuronEventOutputView const& data,
    lola::vx::v3::Chip& chip) const
{
	for (auto const& [row, columns] : data.get_neurons()) {
		for (auto const& cs : columns) {
			for (auto const& column : cs) {
				chip.neuron_block
				    .backends[column.toNeuronEventOutputOnDLS()
				                  .toNeuronBackendConfigBlockOnDLS()
				                  .toCommonNeuronBackendConfigOnDLS()]
				    .set_enable_event_registers(true);
			}
		}
	}
}

template <>
void ExecutionInstanceConfigVisitor::process(
    [[maybe_unused]] signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::MADCReadoutView const& data,
    lola::vx::v3::Chip& chip) const
{
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	// MADCReadoutView inputs size equals 1 or 2
	assert(boost::in_degree(vertex, m_graph.get_graph()) <= 2);

	// first source
	// Determine readout chain configuration
	auto& smux = chip.readout_chain.input_mux[SourceMultiplexerOnReadoutSourceSelection(0)];
	auto const& first_source = data.get_first_source();
	bool const is_odd = first_source.coord.toNeuronColumnOnDLS() % 2;
	auto neuron_even = smux.get_neuron_even();
	neuron_even[first_source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = !is_odd;
	smux.set_neuron_even(neuron_even);
	auto neuron_odd = smux.get_neuron_odd();
	neuron_odd[first_source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = is_odd;
	smux.set_neuron_odd(neuron_odd);
	chip.readout_chain.buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection(0)].enable = true;
	// Configure neuron
	chip.neuron_block.atomic_neurons[first_source.coord].readout.source = first_source.type;
	chip.neuron_block.atomic_neurons[first_source.coord].readout.enable_amplifier = true;
	chip.neuron_block.atomic_neurons[first_source.coord].readout.enable_buffered_access = true;

	// second source
	if (data.get_second_source()) {
		// Determine readout chain configuration
		auto& smux = chip.readout_chain.input_mux[SourceMultiplexerOnReadoutSourceSelection(1)];
		auto const& second_source = *(data.get_second_source());
		bool const is_odd = second_source.coord.toNeuronColumnOnDLS() % 2;
		auto neuron_even = smux.get_neuron_even();
		neuron_even[second_source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = !is_odd;
		smux.set_neuron_even(neuron_even);
		auto neuron_odd = smux.get_neuron_odd();
		neuron_odd[second_source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = is_odd;
		smux.set_neuron_odd(neuron_odd);
		chip.readout_chain.buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection(1)].enable =
		    true;
		// Configure neuron
		chip.neuron_block.atomic_neurons[second_source.coord].readout.source = second_source.type;
		chip.neuron_block.atomic_neurons[second_source.coord].readout.enable_amplifier = true;
		chip.neuron_block.atomic_neurons[second_source.coord].readout.enable_buffered_access = true;
	}

	// configure source selection
	chip.readout_chain.dynamic_mux.input_select_length = data.get_source_selection().period;
	chip.readout_chain.dynamic_mux.initially_selected_input = data.get_source_selection().initial;

	// Configure analog parameters
	// TODO: should come from calibration
	chip.readout_chain.dynamic_mux.i_bias = CapMemCell::Value(500);
	chip.readout_chain.madc.in_500na = CapMemCell::Value(500);
	chip.readout_chain.madc_preamp.i_bias = CapMemCell::Value(500);
	chip.readout_chain.madc_preamp.v_ref = CapMemCell::Value(400);
	chip.readout_chain.pseudo_diff_converter.buffer_bias = CapMemCell::Value(0);
	chip.readout_chain.pseudo_diff_converter.v_ref = CapMemCell::Value(400);
	chip.readout_chain.source_measure_unit.amp_v_ref = CapMemCell::Value(400);
	chip.readout_chain.source_measure_unit.test_voltage = CapMemCell::Value(400);
}

template <>
void ExecutionInstanceConfigVisitor::process(
    [[maybe_unused]] signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::PadReadoutView const& data,
    lola::vx::v3::Chip& chip) const
{
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);

	auto const& source = data.get_source();

	chip.neuron_block.atomic_neurons[source.coord].readout.source = source.type;

	if (data.get_source().enable_buffered) {
		// Determine readout chain configuration
		auto& smux =
		    chip.readout_chain
		        .input_mux[SourceMultiplexerOnReadoutSourceSelection(data.get_coordinate())];
		bool const is_odd = source.coord.toNeuronColumnOnDLS() % 2;
		auto neuron_even = smux.get_neuron_even();
		neuron_even[source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = !is_odd;
		smux.set_neuron_even(neuron_even);
		auto neuron_odd = smux.get_neuron_odd();
		neuron_odd[source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = is_odd;
		smux.set_neuron_odd(neuron_odd);
		chip.readout_chain
		    .buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection(data.get_coordinate())]
		    .enable = true;
		PadMultiplexerConfig::buffer_type buffers;
		buffers.fill(false);
		buffers[SourceMultiplexerOnReadoutSourceSelection(data.get_coordinate())] = true;
		chip.readout_chain.pad_mux[PadMultiplexerConfigOnDLS(data.get_coordinate())]
		    .set_buffer_to_pad(buffers);
		// Configure neuron
		chip.neuron_block.atomic_neurons[source.coord].readout.enable_amplifier = true;
		chip.neuron_block.atomic_neurons[source.coord].readout.enable_buffered_access = true;
	} else {
		// enable unbuffered access to pad
		auto& pad_mux =
		    chip.readout_chain.pad_mux[PadMultiplexerConfigOnDLS(data.get_coordinate())];
		auto neuron_i_stim_mux = pad_mux.get_neuron_i_stim_mux();
		neuron_i_stim_mux.fill(false);
		neuron_i_stim_mux[source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = true;
		pad_mux.set_neuron_i_stim_mux(neuron_i_stim_mux);
		pad_mux.set_neuron_i_stim_mux_to_pad(true);
		// Configure neuron
		chip.neuron_block.atomic_neurons[source.coord].readout.enable_unbuffered_access = true;
	}

	// Configure analog parameters
	for (auto const smux : halco::common::iter_all<SourceMultiplexerOnReadoutSourceSelection>()) {
		chip.readout_chain.buffer_to_pad[smux].amp_i_bias = CapMemCell::Value(1022);
	}
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::SynapseArrayView const& data,
    lola::vx::v3::Chip& chip) const
{
	auto& config = chip;
	auto const& columns = data.get_columns();
	auto const synram = data.get_synram();
	auto const hemisphere = synram.toHemisphereOnDLS();
	for (auto const& var : boost::combine(data.get_weights(), data.get_labels(), data.get_rows())) {
		auto const& weights = boost::get<0>(var);
		auto const& labels = boost::get<1>(var);
		auto const& row = boost::get<2>(var);
		auto& weight_row =
		    config.synapse_blocks[hemisphere.toSynapseBlockOnDLS()].matrix.weights[row];
		auto& label_row =
		    config.synapse_blocks[hemisphere.toSynapseBlockOnDLS()].matrix.labels[row];
		for (size_t index = 0; index < columns.size(); ++index) {
			auto const syn_column = columns[index];
			weight_row[syn_column] = weights[index];
			label_row[syn_column] = labels[index];
		}
	}
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::SynapseArrayViewSparse const& data,
    lola::vx::v3::Chip& chip) const
{
	auto& config = chip;
	auto const& columns = data.get_columns();
	auto const& rows = data.get_rows();
	auto const synram = data.get_synram();
	auto const hemisphere = synram.toHemisphereOnDLS();
	auto& synapse_matrix = config.synapse_blocks[hemisphere.toSynapseBlockOnDLS()].matrix;
	for (auto const& synapse : data.get_synapses()) {
		auto const row = *(rows.begin() + synapse.index_row);
		auto const column = *(columns.begin() + synapse.index_column);
		synapse_matrix.weights[row][column] = synapse.weight;
		synapse_matrix.labels[row][column] = synapse.label;
	}
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor,
    signal_flow::vertex::PlasticityRule const& data,
    lola::vx::v3::Chip& chip) const
{
	// handle setting neuron readout parameters
	for (auto const& neuron_view : data.get_neuron_view_shapes()) {
		for (size_t i = 0; i < neuron_view.columns.size(); ++i) {
			if (neuron_view.neuron_readout_sources.at(i)) {
				auto& atomic_neuron =
				    chip.neuron_block.atomic_neurons[halco::hicann_dls::vx::v3::AtomicNeuronOnDLS(
				        neuron_view.columns.at(i), neuron_view.row)];
				atomic_neuron.readout.source = *neuron_view.neuron_readout_sources.at(i);
				atomic_neuron.readout.enable_amplifier = true;
			}
		}
	}
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::SynapseDriver const& data,
    lola::vx::v3::Chip& chip) const
{
	auto& synapse_driver_config =
	    chip.synapse_driver_blocks[data.get_coordinate().toSynapseDriverBlockOnDLS()]
	        .synapse_drivers[data.get_coordinate().toSynapseDriverOnSynapseDriverBlock()];
	synapse_driver_config.set_row_mode_top(
	    data.get_config().row_modes[halco::hicann_dls::vx::v3::SynapseRowOnSynapseDriver::top]);
	synapse_driver_config.set_row_mode_bottom(
	    data.get_config().row_modes[halco::hicann_dls::vx::v3::SynapseRowOnSynapseDriver::bottom]);
	synapse_driver_config.set_row_address_compare_mask(data.get_config().row_address_compare_mask);
	synapse_driver_config.set_enable_address_out(data.get_config().enable_address_out);
	synapse_driver_config.set_enable_receiver(true);
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::PADIBus const& data,
    lola::vx::v3::Chip& chip) const
{
	auto const bus = data.get_coordinate();
	auto& config =
	    chip.synapse_driver_blocks[bus.toPADIBusBlockOnDLS().toSynapseDriverBlockOnDLS()].padi_bus;
	auto enable_config = config.get_enable_spl1();
	enable_config[bus.toPADIBusOnPADIBusBlock()] = true;
	config.set_enable_spl1(enable_config);
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::CrossbarNode const& data,
    lola::vx::v3::Chip& chip) const
{
	chip.crossbar.nodes[data.get_coordinate()] = data.get_config();
	// enable drop counter for accumulated measure in health info
	chip.crossbar.nodes[data.get_coordinate()].set_enable_drop_counter(true);
	auto enable_event_counter = chip.crossbar.outputs.get_enable_event_counter();
	// enable event counter of output for accumulated measure in health info
	enable_event_counter[data.get_coordinate().toCrossbarOutputOnDLS()] = true;
	chip.crossbar.outputs.set_enable_event_counter(enable_event_counter);
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::BackgroundSpikeSource const& data,
    lola::vx::v3::Chip& chip) const
{
	chip.background_spike_sources[data.get_coordinate()] = data.get_config();
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::NeuronView const& data,
    lola::vx::v3::Chip& chip) const
{
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	size_t i = 0;
	auto const& configs = data.get_configs();
	auto& neurons = chip.neuron_block.atomic_neurons;
	for (auto const column : data.get_columns()) {
		auto const& config = configs.at(i);
		auto const an = AtomicNeuronOnDLS(column, data.get_row());
		auto const neuron_reset = an.toNeuronResetOnDLS();
		if (config.label) {
			neurons[an].event_routing.address = *(config.label);
			neurons[an].event_routing.enable_digital = true;
		} else {
			neurons[an].event_routing.enable_digital = false;
		}
		i++;
	}
	// get incoming synapse columns and enable current switches
	if (boost::in_degree(vertex, m_graph.get_graph()) > 0) {
		std::set<SynapseOnSynapseRow> incoming_synapse_columns;
		auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
		for (auto const in_edge : boost::make_iterator_range(in_edges)) {
			auto const source = boost::source(in_edge, m_graph.get_graph());
			if (std::holds_alternative<signal_flow::vertex::SynapseArrayView>(
			        m_graph.get_vertex_property(source))) {
				auto const& synapses = std::get<signal_flow::vertex::SynapseArrayView>(
				    m_graph.get_vertex_property(source));
				for (auto const& column : synapses.get_columns()) {
					incoming_synapse_columns.insert(column);
				}
			} else {
				auto const& synapses = std::get<signal_flow::vertex::SynapseArrayViewSparse>(
				    m_graph.get_vertex_property(source));
				for (auto const& column : synapses.get_columns()) {
					incoming_synapse_columns.insert(column);
				}
			}
		}
		for (auto const& column : incoming_synapse_columns) {
			chip.neuron_block.current_rows[data.get_row().toColumnCurrentRowOnDLS()]
			    .values[column]
			    .set_enable_synaptic_current_excitatory(true);
			chip.neuron_block.current_rows[data.get_row().toColumnCurrentRowOnDLS()]
			    .values[column]
			    .set_enable_synaptic_current_inhibitory(true);
		}
	}
}

void ExecutionInstanceConfigVisitor::operator()(lola::vx::v3::Chip& chip) const
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceConfigVisitor");

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;

	/** Silence everything which is not set in the graph. */
	for (auto& node : chip.crossbar.nodes) {
		node = haldls::vx::v3::CrossbarNode::drop_all;
	}
	for (auto const& block : iter_all<SynapseDriverBlockOnDLS>()) {
		for (auto const drv : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
			chip.synapse_driver_blocks[block].synapse_drivers[drv].set_row_mode_top(
			    haldls::vx::v3::SynapseDriverConfig::RowMode::disabled);
			chip.synapse_driver_blocks[block].synapse_drivers[drv].set_row_mode_bottom(
			    haldls::vx::v3::SynapseDriverConfig::RowMode::disabled);
			chip.synapse_driver_blocks[block].synapse_drivers[drv].set_enable_receiver(false);
		}
	}
	for (auto const& row : iter_all<ColumnCurrentRowOnDLS>()) {
		for (auto const col : iter_all<SynapseOnSynapseRow>()) {
			chip.neuron_block.current_rows[row].values[col].set_enable_synaptic_current_excitatory(
			    false);
			chip.neuron_block.current_rows[row].values[col].set_enable_synaptic_current_inhibitory(
			    false);
		}
	}
	for (auto const backend : iter_all<CommonNeuronBackendConfigOnDLS>()) {
		chip.neuron_block.backends[backend].set_enable_event_registers(false);
	}
	{
		for (auto const& block : iter_all<SynapseBlockOnDLS>()) {
			for (auto const& row : iter_all<SynapseRowOnSynram>()) {
				chip.synapse_blocks[block].matrix.weights[row].fill(
				    lola::vx::v3::SynapseMatrix::Weight(0));
				chip.synapse_blocks[block].matrix.labels[row].fill(
				    lola::vx::v3::SynapseMatrix::Label(0));
			}
		}
	}

	auto const execution_instance_vertex =
	    m_graph.get_execution_instance_map().right.at(m_execution_instance);
	for (auto const p : boost::make_iterator_range(
	         m_graph.get_vertex_descriptor_map().right.equal_range(execution_instance_vertex))) {
		auto const vertex = p.second;
		std::visit(
		    [&](auto const& value) {
			    hate::Timer timer;
			    process(vertex, value, chip);
			    LOG4CXX_TRACE(
			        logger, "process(): Processed "
			                    << hate::name<hate::remove_all_qualifiers_t<decltype(value)>>()
			                    << " in " << timer.print() << ".");
		    },
		    m_graph.get_vertex_property(vertex));
	}
}

} // namespace grenade::vx::execution::detail
