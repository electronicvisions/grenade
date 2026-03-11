#include "grenade/vx/execution/detail/execution_instance_chip_snippet_config_visitor.h"

#include "grenade/vx/execution/detail/ppu_program_generator.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/signal_flow/vertex/background_spike_source.h"
#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "grenade/vx/signal_flow/vertex/input_routing_table.h"
#include "grenade/vx/signal_flow/vertex/madc_readout.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "grenade/vx/signal_flow/vertex/output_routing_table.h"
#include "grenade/vx/signal_flow/vertex/pad_readout.h"
#include "grenade/vx/signal_flow/vertex/padi_bus.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"
#include "grenade/vx/signal_flow/vertex/synapse_driver.h"
#include "halco/hicann-dls/vx/v3/readout.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/padi.h"
#include "haldls/vx/v3/readout.h"
#include "hate/timer.h"
#include "hate/type_index.h"
#include "hate/type_traits.h"
#include "hate/variant.h"
#include "lola/vx/cadc.h"
#include "lola/vx/ppu.h"
#include <filesystem>
#include <iterator>
#include <vector>
#include <boost/range/combine.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail {

ExecutionInstanceChipSnippetConfigVisitor::ExecutionInstanceChipSnippetConfigVisitor(
    grenade::common::LinkedTopology const& topology,
    grenade::common::VertexOnTopology const& execution_instance_vertex_descriptor,
    grenade::common::InputData const& input_data,
    common::ChipOnConnection const& chip_on_connection) :
    m_topology(topology),
    m_execution_instance_vertex_descriptor(execution_instance_vertex_descriptor),
    m_input_data(input_data),
    m_chip_on_connection(chip_on_connection),
    m_logger(log4cxx::Logger::getLogger("grenade.ExecutionInstanceChipSnippetConfigVisitor"))
{
	register_process<signal_flow::vertex::BackgroundSpikeSource>();
	register_process<signal_flow::vertex::CADCMembraneReadoutView>();
	register_process<signal_flow::vertex::CrossbarNode>();
	register_process<signal_flow::vertex::MADCReadoutView>();
	register_process<signal_flow::vertex::PadReadoutView>();
	register_process<signal_flow::vertex::PADIBus>();
	register_process<signal_flow::vertex::SynapseArrayView>();
	register_process<signal_flow::vertex::SynapseArrayViewSparse>();
	register_process<signal_flow::vertex::SynapseDriver>();
	register_process<signal_flow::vertex::NeuronView>();
	register_process<signal_flow::vertex::OutputRoutingTable>();
	register_process<signal_flow::vertex::InputRoutingTable>();
}

template <typename T>
void ExecutionInstanceChipSnippetConfigVisitor::register_process()
{
	m_visitor.emplace(
	    std::type_index(typeid(T)), [&](grenade::common::VertexOnTopology const& vertex_descriptor,
	                                    grenade::common::Vertex const& vertex, System& system) {
		    hate::Timer timer;
		    process(vertex_descriptor, static_cast<T const&>(vertex), system);
		    LOG4CXX_TRACE(
		        m_logger, "process(): Processed " << hate::name<hate::remove_all_qualifiers_t<T>>()
		                                          << " in " << timer.print() << ".");
	    });
}

template <typename T>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const /* vertex */,
    T const& /* data */,
    System& /* system */) const
{
	// Spezialize for types which change static configuration
}

template <>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const /* vertex */,
    signal_flow::vertex::CADCMembraneReadoutView const& data,
    System& system) const
{
	auto& chip =
	    std::visit([](auto& system) -> lola::vx::v3::Chip& { return system.chip; }, system);
	// Configure neurons
	for (size_t i = 0; i < data.get_columns().size(); ++i) {
		halco::hicann_dls::vx::v3::AtomicNeuronOnDLS neuron(
		    data.get_columns().at(i).toNeuronColumnOnDLS(), data.get_synram().toNeuronRowOnDLS());
		chip.neuron_block.atomic_neurons[neuron].readout.enable_amplifier = true;
	}
}

template <>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::MADCReadoutView const& data,
    System& system) const
{
	auto& chip =
	    std::visit([](auto& system) -> lola::vx::v3::Chip& { return system.chip; }, system);
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	// first source
	// Determine readout chain configuration
	auto& smux = chip.readout_chain.input_mux[SourceMultiplexerOnReadoutSourceSelection(0)];
	auto const& first_source = data.get_first_source();
	bool const is_odd = first_source.toNeuronColumnOnDLS() % 2;
	auto neuron_even = smux.get_neuron_even();
	neuron_even[first_source.toNeuronRowOnDLS().toHemisphereOnDLS()] = !is_odd;
	smux.set_neuron_even(neuron_even);
	auto neuron_odd = smux.get_neuron_odd();
	neuron_odd[first_source.toNeuronRowOnDLS().toHemisphereOnDLS()] = is_odd;
	smux.set_neuron_odd(neuron_odd);
	chip.readout_chain.buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection(0)].enable = true;
	// Configure neuron
	chip.neuron_block.atomic_neurons[first_source].readout.enable_amplifier = true;
	chip.neuron_block.atomic_neurons[first_source].readout.enable_buffered_access = true;

	// second source
	if (data.get_second_source()) {
		// Determine readout chain configuration
		auto& smux = chip.readout_chain.input_mux[SourceMultiplexerOnReadoutSourceSelection(1)];
		auto const& second_source = *(data.get_second_source());
		bool const is_odd = second_source.toNeuronColumnOnDLS() % 2;
		auto neuron_even = smux.get_neuron_even();
		neuron_even[second_source.toNeuronRowOnDLS().toHemisphereOnDLS()] = !is_odd;
		smux.set_neuron_even(neuron_even);
		auto neuron_odd = smux.get_neuron_odd();
		neuron_odd[second_source.toNeuronRowOnDLS().toHemisphereOnDLS()] = is_odd;
		smux.set_neuron_odd(neuron_odd);
		chip.readout_chain.buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection(1)].enable =
		    true;
		// Configure neuron
		chip.neuron_block.atomic_neurons[second_source].readout.enable_amplifier = true;
		chip.neuron_block.atomic_neurons[second_source].readout.enable_buffered_access = true;
	}

	auto const& parameterization =
	    dynamic_cast<signal_flow::vertex::MADCReadoutView::Parameterization const&>(
	        m_input_data.ports.get(grenade::common::PortOnTopology{vertex, 1}));

	// configure source selection
	chip.readout_chain.dynamic_mux.input_select_length = parameterization.period;
	chip.readout_chain.dynamic_mux.initially_selected_input = parameterization.initial;

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
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::PadReadoutView const& data,
    System& system) const
{
	auto& chip =
	    std::visit([](auto& system) -> lola::vx::v3::Chip& { return system.chip; }, system);
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	auto const& source = data.source;

	auto const& parameterization =
	    dynamic_cast<signal_flow::vertex::PadReadoutView::Parameterization const&>(
	        m_input_data.ports.get(grenade::common::PortOnTopology{vertex, 1}));

	if (parameterization.enable_buffered) {
		// Determine readout chain configuration
		auto& smux = chip.readout_chain
		                 .input_mux[SourceMultiplexerOnReadoutSourceSelection(data.coordinate)];
		bool const is_odd = source.toNeuronColumnOnDLS() % 2;
		auto neuron_even = smux.get_neuron_even();
		neuron_even[source.toNeuronRowOnDLS().toHemisphereOnDLS()] = !is_odd;
		smux.set_neuron_even(neuron_even);
		auto neuron_odd = smux.get_neuron_odd();
		neuron_odd[source.toNeuronRowOnDLS().toHemisphereOnDLS()] = is_odd;
		smux.set_neuron_odd(neuron_odd);
		chip.readout_chain.buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection(data.coordinate)]
		    .enable = true;
		PadMultiplexerConfig::buffer_type buffers;
		buffers.fill(false);
		buffers[SourceMultiplexerOnReadoutSourceSelection(data.coordinate)] = true;
		chip.readout_chain.pad_mux[PadMultiplexerConfigOnDLS(data.coordinate)].set_buffer_to_pad(
		    buffers);
		// Configure neuron
		chip.neuron_block.atomic_neurons[source].readout.enable_amplifier = true;
		chip.neuron_block.atomic_neurons[source].readout.enable_buffered_access = true;
	} else {
		// enable unbuffered access to pad
		auto& pad_mux = chip.readout_chain.pad_mux[PadMultiplexerConfigOnDLS(data.coordinate)];
		auto neuron_i_stim_mux = pad_mux.get_neuron_i_stim_mux();
		neuron_i_stim_mux.fill(false);
		neuron_i_stim_mux[source.toNeuronRowOnDLS().toHemisphereOnDLS()] = true;
		pad_mux.set_neuron_i_stim_mux(neuron_i_stim_mux);
		pad_mux.set_neuron_i_stim_mux_to_pad(true);
		// Configure neuron
		chip.neuron_block.atomic_neurons[source].readout.enable_unbuffered_access = true;
	}

	// Configure analog parameters
	for (auto const smux : halco::common::iter_all<SourceMultiplexerOnReadoutSourceSelection>()) {
		chip.readout_chain.buffer_to_pad[smux].amp_i_bias = CapMemCell::Value(1022);
	}
}

template <>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::SynapseArrayView const& data,
    System& system) const
{
	auto& chip =
	    std::visit([](auto& system) -> lola::vx::v3::Chip& { return system.chip; }, system);
	auto& config = chip;
	auto const& columns = data.get_columns();
	auto const synram = data.get_synram();
	auto const hemisphere = synram.toHemisphereOnDLS();
	auto const& parameterization =
	    dynamic_cast<signal_flow::vertex::SynapseArrayView::Parameterization const&>(
	        m_input_data.ports.get(grenade::common::PortOnTopology{vertex, 1}));
	for (auto const& var :
	     boost::combine(parameterization.weights, parameterization.labels, data.get_rows())) {
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
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::SynapseArrayViewSparse const& data,
    System& system) const
{
	auto& chip =
	    std::visit([](auto& system) -> lola::vx::v3::Chip& { return system.chip; }, system);
	auto& config = chip;
	auto const& columns = data.get_columns();
	auto const& rows = data.get_rows();
	auto const synram = data.get_synram();
	auto const hemisphere = synram.toHemisphereOnDLS();
	auto& synapse_matrix = config.synapse_blocks[hemisphere.toSynapseBlockOnDLS()].matrix;
	auto const& parameterization =
	    dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse::Parameterization const&>(
	        m_input_data.ports.get(grenade::common::PortOnTopology{vertex, 1}));
	for (size_t i = 0; auto const& synapse : data.get_synapses()) {
		auto const row = *(rows.begin() + synapse.index_row);
		auto const column = *(columns.begin() + synapse.index_column);
		synapse_matrix.weights[row][column] = parameterization.weights.at(i);
		synapse_matrix.labels[row][column] = parameterization.labels.at(i);
		i++;
	}
}

template <>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology,
    signal_flow::vertex::PlasticityRule const& data,
    System& system) const
{
	auto& chip =
	    std::visit([](auto& system) -> lola::vx::v3::Chip& { return system.chip; }, system);
	// Set CADC switches for reading from the synapses' correlation sensors or neurons' membrane
	for (auto const coord : halco::common::iter_all<halco::hicann_dls::vx::CADCOnDLS>()) {
		for (auto const column :
		     halco::common::iter_all<halco::hicann_dls::vx::CADCChannelColumnOnSynram>()) {
			chip.cadc_readout_chains[coord].channels_causal[column].enable_connect_correlation =
			    true;
			chip.cadc_readout_chains[coord].channels_acausal[column].enable_connect_correlation =
			    true;
		}
	}
	// handle setting neuron readout amplifier
	for (auto const& neuron_view : data.get_neuron_view_shapes()) {
		for (size_t i = 0; i < neuron_view.columns.size(); ++i) {
			auto& atomic_neuron =
			    chip.neuron_block.atomic_neurons[halco::hicann_dls::vx::v3::AtomicNeuronOnDLS(
			        neuron_view.columns.at(i), neuron_view.row)];
			atomic_neuron.readout.enable_amplifier = true;
		}
	}
}

template <>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::SynapseDriver const& data,
    System& system) const
{
	auto const& parameterization =
	    dynamic_cast<signal_flow::vertex::SynapseDriver::Parameterization const&>(
	        m_input_data.ports.get(grenade::common::PortOnTopology{vertex, 1}));
	auto& chip =
	    std::visit([](auto& system) -> lola::vx::v3::Chip& { return system.chip; }, system);
	auto& synapse_driver_config =
	    chip.synapse_driver_blocks[data.coordinate.toSynapseDriverBlockOnDLS()]
	        .synapse_drivers[data.coordinate.toSynapseDriverOnSynapseDriverBlock()];
	synapse_driver_config.set_row_mode_top(
	    parameterization.row_modes[halco::hicann_dls::vx::v3::SynapseRowOnSynapseDriver::top]);
	synapse_driver_config.set_row_mode_bottom(
	    parameterization.row_modes[halco::hicann_dls::vx::v3::SynapseRowOnSynapseDriver::bottom]);
	synapse_driver_config.set_row_address_compare_mask(parameterization.row_address_compare_mask);
	synapse_driver_config.set_enable_address_out(parameterization.enable_address_out);
	synapse_driver_config.set_enable_receiver(true);
}

template <>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const /* vertex */,
    signal_flow::vertex::PADIBus const& data,
    System& system) const
{
	auto& chip =
	    std::visit([](auto& system) -> lola::vx::v3::Chip& { return system.chip; }, system);
	auto const bus = data.coordinate;
	auto& config =
	    chip.synapse_driver_blocks[bus.toPADIBusBlockOnDLS().toSynapseDriverBlockOnDLS()].padi_bus;
	auto enable_config = config.get_enable_spl1();
	enable_config[bus.toPADIBusOnPADIBusBlock()] = true;
	config.set_enable_spl1(enable_config);
}

template <>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::CrossbarNode const& data,
    System& system) const
{
	auto& chip =
	    std::visit([](auto& system) -> lola::vx::v3::Chip& { return system.chip; }, system);
	auto const& parameterization =
	    dynamic_cast<signal_flow::vertex::CrossbarNode::Parameterization const&>(
	        m_input_data.ports.get(grenade::common::PortOnTopology{vertex, 1}));
	chip.crossbar.nodes[data.coordinate] = parameterization.config;
	// enable drop counter for accumulated measure in health info
	chip.crossbar.nodes[data.coordinate].set_enable_drop_counter(true);
	auto enable_event_counter = chip.crossbar.outputs.get_enable_event_counter();
	// enable event counter of output for accumulated measure in health info
	enable_event_counter[data.coordinate.toCrossbarOutputOnDLS()] = true;
	chip.crossbar.outputs.set_enable_event_counter(enable_event_counter);
}

template <>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::BackgroundSpikeSource const& data,
    System& system) const
{
	auto& chip =
	    std::visit([](auto& system) -> lola::vx::v3::Chip& { return system.chip; }, system);
	auto const& parameterization =
	    dynamic_cast<signal_flow::vertex::BackgroundSpikeSource::Parameterization const&>(
	        m_input_data.ports.get(grenade::common::PortOnTopology{vertex, 0}));
	chip.background_spike_sources[data.coordinate] = parameterization.config;
}

// TODO(SpikeIO)
// template <>
// void ExecutionInstanceChipSnippetConfigVisitor::process(
//     signal_flow::Graph::vertex_descriptor const /* vertex */,
//     signal_flow::vertex::SpikeIOSourcePopulation const& data,
//     System& system) const
//{
//	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceChipSnippetConfigVisitor");
//	auto* sys = std::get_if<lola::vx::v3::ChipAndSinglechipFPGA>(&system);
//	if (!sys) {
//		throw std::logic_error("SpikeIOSource requires ChipAndSingleChipFPGA!");
//	}
//	auto& fpga = sys->fpga;
//
//	auto const& config = data.get_config();
//	LOG4CXX_TRACE(
//	    logger, "process(SpikeIOSourcePopulation): enable_tx="
//	                << config.get_enable_tx() << " enable_rx=" << config.get_enable_rx()
//	                << " loopback=" << config.get_enable_internal_loopback() << "dataratescaler"
//	                << config.get_data_rate_scaler());
//
//
//	auto const& in = data.get_input_routing();
//	for (auto const& [coord, lbl] : in) {
//		LOG4CXX_TRACE(
//		    logger, "    input_routing[" << coord << "] -> label=" << lbl
//		                                 << " neuron_label=" << lbl.get_neuron_label()
//		                                 << " row=" << lbl.get_row_select_address()
//		                                 << " synapse=" << lbl.get_synapse_label()
//		                                 << " spl1=" << lbl.get_spl1_address());
//	}
//
//	auto& fpga_cfg = fpga.spikeio_config;
//
//	fpga_cfg.set_enable_tx(fpga_cfg.get_enable_tx() || config.get_enable_tx());
//	fpga_cfg.set_enable_rx(fpga_cfg.get_enable_rx() || config.get_enable_rx());
//	fpga_cfg.set_enable_internal_loopback(
//	    fpga_cfg.get_enable_internal_loopback() || config.get_enable_internal_loopback());
//	fpga_cfg.set_data_rate_scaler(config.get_data_rate_scaler());
//	// data rate consistency check moved to network_builder
//	{
//		for (auto const& [serial_addr, lbl] : data.get_input_routing()) {
//			auto const coord = serial_addr.toSpikeIOInputRouteOnFPGA();
//			auto& entry = fpga.spikeio_input_routes[coord];
//			auto const cur = entry.get_target();
//
//			if (cur != haldls::vx::SpikeIOInputRoute::SILENT && cur != lbl) {
//				std::ostringstream oss;
//				oss << "SpikeIO input_routing conflict at coord " << coord;
//				throw std::runtime_error(oss.str());
//			}
//
//			entry.set_target(lbl);
//
//			auto const after = entry.get_target();
//			if (after != lbl) {
//				throw std::logic_error("SpikeIO input route did not stick in LoLA container");
//			}
//
//			LOG4CXX_TRACE(
//			    logger, "SpikeIO RX: " << serial_addr << " -> " << coord << " = " << after);
//		}
//	}
// }

template <>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::NeuronView const& data,
    System& system) const
{
	auto& chip =
	    std::visit([](auto& system) -> lola::vx::v3::Chip& { return system.chip; }, system);
	auto const& parameterization =
	    dynamic_cast<signal_flow::vertex::NeuronView::Parameterization const&>(
	        m_input_data.ports.get(grenade::common::PortOnTopology{vertex, 1}));
	for (auto const& column : data.get_columns()) {
		chip.neuron_block
		    .backends[column.toNeuronEventOutputOnDLS()
		                  .toNeuronBackendConfigBlockOnDLS()
		                  .toCommonNeuronBackendConfigOnDLS()]
		    .set_enable_event_registers(true);
	}

	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	size_t i = 0;
	auto const& configs = parameterization.configs;
	auto& neurons = chip.neuron_block.atomic_neurons;
	for (auto const column : data.get_columns()) {
		auto const& config = configs.at(i);
		auto& atomic_neuron = neurons[AtomicNeuronOnDLS(column, data.row)];
		auto old_readout = atomic_neuron.readout;
		atomic_neuron = config.atomic_neuron_config;
		// leave readout amplifier settings untouched, since they are set by other vertices
		// (adcs, plasticity)
		atomic_neuron.readout.enable_amplifier = old_readout.enable_amplifier;
		atomic_neuron.readout.enable_buffered_access = old_readout.enable_buffered_access;
		atomic_neuron.readout.enable_unbuffered_access = old_readout.enable_unbuffered_access;
		i++;
	}
	// get incoming synapse columns and enable current switches
	if (m_topology.get_reference().in_degree(vertex) > 0) {
		std::set<SynapseOnSynapseRow> incoming_synapse_columns;
		for (auto const in_edge : m_topology.get_reference().in_edges(vertex)) {
			auto const source = m_topology.get_reference().source(in_edge);
			if (auto const synapses = dynamic_cast<signal_flow::vertex::SynapseArrayView const*>(
			        &m_topology.get_reference().get(source));
			    synapses) {
				for (auto const& column : synapses->get_columns()) {
					incoming_synapse_columns.insert(column);
				}
			} else {
				auto const& synapses_sparse =
				    dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse const&>(
				        m_topology.get_reference().get(source));
				for (auto const& column : synapses_sparse.get_columns()) {
					incoming_synapse_columns.insert(column);
				}
			}
		}
		for (auto const& column : incoming_synapse_columns) {
			chip.neuron_block.current_rows[data.row.toColumnCurrentRowOnDLS()]
			    .values[column]
			    .set_enable_synaptic_current_excitatory(true);
			chip.neuron_block.current_rows[data.row.toColumnCurrentRowOnDLS()]
			    .values[column]
			    .set_enable_synaptic_current_inhibitory(true);
		}
	}
}

template <>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::OutputRoutingTable const&,
    System& system) const
{
	auto const& parameterization =
	    dynamic_cast<signal_flow::vertex::OutputRoutingTable::Parameterization const&>(
	        m_input_data.ports.get(grenade::common::PortOnTopology{vertex, 1}));

	if (!std::holds_alternative<lola::vx::v3::ChipAndMultichipJboaLeafFPGA>(system)) {
		throw std::logic_error(
		    "Output routing table vertex on hardware graph but not on a JBOA-Multichip-setup.");
	}

	auto& multichip_system = std::get<lola::vx::v3::ChipAndMultichipJboaLeafFPGA>(system);
	for (auto const& [on_chip_label, routing_label] : parameterization.entries) {
		auto& routing_table_entry =
		    multichip_system.fpga.output_routing_table.entries[on_chip_label];
		routing_table_entry.set_label(routing_label);
		routing_table_entry.set_enable(true);
	}
}

template <>
void ExecutionInstanceChipSnippetConfigVisitor::process(
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::InputRoutingTable const&,
    System& system) const
{
	auto const& parameterization =
	    dynamic_cast<signal_flow::vertex::InputRoutingTable::Parameterization const&>(
	        m_input_data.ports.get(grenade::common::PortOnTopology{vertex, 1}));

	if (!std::holds_alternative<lola::vx::v3::ChipAndMultichipJboaLeafFPGA>(system)) {
		throw std::logic_error(
		    "Input routing table vertex on hardware graph but not on a JBOA-Multichip-setup.");
	}

	auto& multichip_system = std::get<lola::vx::v3::ChipAndMultichipJboaLeafFPGA>(system);
	for (auto const& [routing_label, on_chip_label] : parameterization.entries) {
		auto& routing_table_entry =
		    multichip_system.fpga.input_routing_table.entries[routing_label];
		routing_table_entry.set_label(on_chip_label);
		routing_table_entry.set_enable(true);
	}
}

void ExecutionInstanceChipSnippetConfigVisitor::operator()(System& system) const
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;

	auto& chip = std::visit([](auto& sys) -> lola::vx::v3::Chip& { return sys.chip; }, system);

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

	for (auto const inter_topology_hyper_edge_descriptor :
	     m_topology.inter_graph_hyper_edges_by_linked(m_execution_instance_vertex_descriptor)) {
		for (auto const& vertex_descriptor :
		     m_topology.references(inter_topology_hyper_edge_descriptor)) {
			auto const& vertex = m_topology.get_reference().get(vertex_descriptor);
			if (auto const entity_on_chip =
			        dynamic_cast<signal_flow::vertex::EntityOnChip const*>(&vertex);
			    entity_on_chip && entity_on_chip->chip_on_connection == m_chip_on_connection) {
				if (m_visitor.contains(std::type_index(typeid(vertex)))) {
					m_visitor.at(std::type_index(typeid(vertex)))(
					    vertex_descriptor, vertex, system);
				}
			}
		}
	}
}

} // namespace grenade::vx::execution::detail
