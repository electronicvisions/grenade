#include "grenade/vx/execution/detail/execution_instance_config_visitor.h"

#include "grenade/vx/common/execution_instance_id.h"
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

ExecutionInstanceConfigVisitor::PpuUsage::PpuUsage(
    ExecutionInstanceConfigVisitor::PpuUsage&& other) :
    has_periodic_cadc_readout(other.has_periodic_cadc_readout),
    has_cadc_readout(other.has_cadc_readout),
    plasticity_rules(std::move(other.plasticity_rules))
{}

ExecutionInstanceConfigVisitor::PpuUsage& ExecutionInstanceConfigVisitor::PpuUsage::operator=(
    ExecutionInstanceConfigVisitor::PpuUsage&& other)
{
	has_periodic_cadc_readout = other.has_periodic_cadc_readout;
	has_cadc_readout = other.has_cadc_readout;
	plasticity_rules = std::move(other.plasticity_rules);
	return *this;
}

ExecutionInstanceConfigVisitor::PpuUsage& ExecutionInstanceConfigVisitor::PpuUsage::operator+=(
    ExecutionInstanceConfigVisitor::PpuUsage&& other)
{
	has_periodic_cadc_readout = has_periodic_cadc_readout || other.has_periodic_cadc_readout;
	has_cadc_readout = has_cadc_readout || other.has_cadc_readout;
	plasticity_rules.insert(
	    plasticity_rules.begin(), std::make_move_iterator(other.plasticity_rules.begin()),
	    std::make_move_iterator(other.plasticity_rules.end()));
	return *this;
}

ExecutionInstanceConfigVisitor::ExecutionInstanceConfigVisitor(
    signal_flow::Graph const& graph,
    common::ExecutionInstanceID const& execution_instance,
    lola::vx::v3::Chip& chip_config) :
    m_graph(graph),
    m_execution_instance(execution_instance),
    m_config(chip_config),
    m_has_periodic_cadc_readout(false),
    m_has_cadc_readout(false)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	m_used_madc = false;

	/** Silence everything which is not set in the graph. */
	for (auto& node : m_config.crossbar.nodes) {
		node = haldls::vx::v3::CrossbarNode::drop_all;
	}
	for (auto const& block : iter_all<SynapseDriverBlockOnDLS>()) {
		for (auto const drv : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
			m_config.synapse_driver_blocks[block].synapse_drivers[drv].set_row_mode_top(
			    haldls::vx::v3::SynapseDriverConfig::RowMode::disabled);
			m_config.synapse_driver_blocks[block].synapse_drivers[drv].set_row_mode_bottom(
			    haldls::vx::v3::SynapseDriverConfig::RowMode::disabled);
			m_config.synapse_driver_blocks[block].synapse_drivers[drv].set_enable_receiver(false);
		}
	}
	for (auto const& row : iter_all<ColumnCurrentRowOnDLS>()) {
		for (auto const col : iter_all<SynapseOnSynapseRow>()) {
			m_config.neuron_block.current_rows[row]
			    .values[col]
			    .set_enable_synaptic_current_excitatory(false);
			m_config.neuron_block.current_rows[row]
			    .values[col]
			    .set_enable_synaptic_current_inhibitory(false);
		}
	}
	for (auto const backend : iter_all<CommonNeuronBackendConfigOnDLS>()) {
		m_config.neuron_block.backends[backend].set_enable_event_registers(false);
	}
	{
		for (auto const& block : iter_all<SynapseBlockOnDLS>()) {
			for (auto const& row : iter_all<SynapseRowOnSynram>()) {
				m_config.synapse_blocks[block].matrix.weights[row].fill(
				    lola::vx::v3::SynapseMatrix::Weight(0));
				m_config.synapse_blocks[block].matrix.labels[row].fill(
				    lola::vx::v3::SynapseMatrix::Label(0));
			}
		}
	}
}

template <typename T>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */, T const& /* data */)
{
	// Spezialize for types which change static configuration
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::CADCMembraneReadoutView const& data)
{
	// Configure neurons
	for (size_t i = 0; i < data.get_columns().size(); ++i) {
		for (size_t j = 0; j < data.get_columns().at(i).size(); ++j) {
			halco::hicann_dls::vx::v3::AtomicNeuronOnDLS neuron(
			    data.get_columns().at(i).at(j).toNeuronColumnOnDLS(),
			    data.get_synram().toNeuronRowOnDLS());
			m_config.neuron_block.atomic_neurons[neuron].readout.source =
			    data.get_sources().at(i).at(j);
			m_config.neuron_block.atomic_neurons[neuron].readout.enable_amplifier = true;
		}
	}

	m_has_periodic_cadc_readout =
	    data.get_mode() == signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic;
	m_has_cadc_readout = true;
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const,
    signal_flow::vertex::NeuronEventOutputView const& data)
{
	for (auto const& [row, columns] : data.get_neurons()) {
		for (auto const& cs : columns) {
			for (auto const& column : cs) {
				m_config.neuron_block
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
    signal_flow::vertex::MADCReadoutView const& data)
{
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	// MADCReadoutView inputs size equals 1 or 2
	assert(boost::in_degree(vertex, m_graph.get_graph()) <= 2);

	// first source
	// Determine readout chain configuration
	auto& smux = m_config.readout_chain.input_mux[SourceMultiplexerOnReadoutSourceSelection(0)];
	auto const& first_source = data.get_first_source();
	bool const is_odd = first_source.coord.toNeuronColumnOnDLS() % 2;
	auto neuron_even = smux.get_neuron_even();
	neuron_even[first_source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = !is_odd;
	smux.set_neuron_even(neuron_even);
	auto neuron_odd = smux.get_neuron_odd();
	neuron_odd[first_source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = is_odd;
	smux.set_neuron_odd(neuron_odd);
	m_config.readout_chain.buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection(0)].enable =
	    true;
	// Configure neuron
	m_config.neuron_block.atomic_neurons[first_source.coord].readout.source = first_source.type;
	m_config.neuron_block.atomic_neurons[first_source.coord].readout.enable_amplifier = true;
	m_config.neuron_block.atomic_neurons[first_source.coord].readout.enable_buffered_access = true;

	// second source
	if (data.get_second_source()) {
		// Determine readout chain configuration
		auto& smux = m_config.readout_chain.input_mux[SourceMultiplexerOnReadoutSourceSelection(1)];
		auto const& second_source = *(data.get_second_source());
		bool const is_odd = second_source.coord.toNeuronColumnOnDLS() % 2;
		auto neuron_even = smux.get_neuron_even();
		neuron_even[second_source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = !is_odd;
		smux.set_neuron_even(neuron_even);
		auto neuron_odd = smux.get_neuron_odd();
		neuron_odd[second_source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = is_odd;
		smux.set_neuron_odd(neuron_odd);
		m_config.readout_chain.buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection(1)].enable =
		    true;
		// Configure neuron
		m_config.neuron_block.atomic_neurons[second_source.coord].readout.source =
		    second_source.type;
		m_config.neuron_block.atomic_neurons[second_source.coord].readout.enable_amplifier = true;
		m_config.neuron_block.atomic_neurons[second_source.coord].readout.enable_buffered_access =
		    true;
	}

	// configure source selection
	m_config.readout_chain.dynamic_mux.input_select_length = data.get_source_selection().period;
	m_config.readout_chain.dynamic_mux.initially_selected_input =
	    data.get_source_selection().initial;

	// Configure analog parameters
	// TODO: should come from calibration
	m_config.readout_chain.dynamic_mux.i_bias = CapMemCell::Value(500);
	m_config.readout_chain.madc.in_500na = CapMemCell::Value(500);
	m_config.readout_chain.madc_preamp.i_bias = CapMemCell::Value(500);
	m_config.readout_chain.madc_preamp.v_ref = CapMemCell::Value(400);
	m_config.readout_chain.pseudo_diff_converter.buffer_bias = CapMemCell::Value(0);
	m_config.readout_chain.pseudo_diff_converter.v_ref = CapMemCell::Value(400);
	m_config.readout_chain.source_measure_unit.amp_v_ref = CapMemCell::Value(400);
	m_config.readout_chain.source_measure_unit.test_voltage = CapMemCell::Value(400);

	m_used_madc = true;
}

template <>
void ExecutionInstanceConfigVisitor::process(
    [[maybe_unused]] signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::PadReadoutView const& data)
{
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);

	auto const& source = data.get_source();

	m_config.neuron_block.atomic_neurons[source.coord].readout.source = source.type;

	if (data.get_source().enable_buffered) {
		// Determine readout chain configuration
		auto& smux =
		    m_config.readout_chain
		        .input_mux[SourceMultiplexerOnReadoutSourceSelection(data.get_coordinate())];
		bool const is_odd = source.coord.toNeuronColumnOnDLS() % 2;
		auto neuron_even = smux.get_neuron_even();
		neuron_even[source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = !is_odd;
		smux.set_neuron_even(neuron_even);
		auto neuron_odd = smux.get_neuron_odd();
		neuron_odd[source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = is_odd;
		smux.set_neuron_odd(neuron_odd);
		m_config.readout_chain
		    .buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection(data.get_coordinate())]
		    .enable = true;
		PadMultiplexerConfig::buffer_type buffers;
		buffers.fill(false);
		buffers[SourceMultiplexerOnReadoutSourceSelection(data.get_coordinate())] = true;
		m_config.readout_chain.pad_mux[PadMultiplexerConfigOnDLS(data.get_coordinate())]
		    .set_buffer_to_pad(buffers);
		// Configure neuron
		m_config.neuron_block.atomic_neurons[source.coord].readout.enable_amplifier = true;
		m_config.neuron_block.atomic_neurons[source.coord].readout.enable_buffered_access = true;
	} else {
		// enable unbuffered access to pad
		auto& pad_mux =
		    m_config.readout_chain.pad_mux[PadMultiplexerConfigOnDLS(data.get_coordinate())];
		auto neuron_i_stim_mux = pad_mux.get_neuron_i_stim_mux();
		neuron_i_stim_mux.fill(false);
		neuron_i_stim_mux[source.coord.toNeuronRowOnDLS().toHemisphereOnDLS()] = true;
		pad_mux.set_neuron_i_stim_mux(neuron_i_stim_mux);
		pad_mux.set_neuron_i_stim_mux_to_pad(true);
		// Configure neuron
		m_config.neuron_block.atomic_neurons[source.coord].readout.enable_unbuffered_access = true;
	}

	// Configure analog parameters
	for (auto const smux : halco::common::iter_all<SourceMultiplexerOnReadoutSourceSelection>()) {
		m_config.readout_chain.buffer_to_pad[smux].amp_i_bias = CapMemCell::Value(1022);
	}
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::SynapseArrayView const& data)
{
	auto& config = m_config;
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
    signal_flow::vertex::SynapseArrayViewSparse const& data)
{
	auto& config = m_config;
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
    signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::PlasticityRule const& data)
{
	auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
	// convert the plasticity rule input vertices to their respective on-PPU handles to transfer
	// onto the PPUs
	std::vector<std::pair<halco::hicann_dls::vx::v3::SynramOnDLS, ppu::SynapseArrayViewHandle>>
	    synapses;
	std::vector<std::pair<halco::hicann_dls::vx::v3::NeuronRowOnDLS, ppu::NeuronViewHandle>>
	    neurons;
	for (auto const in_edge : boost::make_iterator_range(in_edges)) {
		// extract on-PPU handles from source vertex properties
		auto const& in_vertex_property =
		    m_graph.get_vertex_property(boost::source(in_edge, m_graph.get_graph()));
		if (std::holds_alternative<signal_flow::vertex::SynapseArrayViewSparse>(
		        in_vertex_property)) {
			auto const& view =
			    std::get<signal_flow::vertex::SynapseArrayViewSparse>(in_vertex_property);
			synapses.push_back({view.get_synram(), view.toSynapseArrayViewHandle()});
		} else if (std::holds_alternative<signal_flow::vertex::NeuronView>(in_vertex_property)) {
			auto const& view = std::get<signal_flow::vertex::NeuronView>(in_vertex_property);
			neurons.push_back({view.get_row(), view.toNeuronViewHandle()});
		}
		// handle setting neuron readout parameters
		for (auto const& neuron_view : data.get_neuron_view_shapes()) {
			for (size_t i = 0; i < neuron_view.columns.size(); ++i) {
				if (neuron_view.neuron_readout_sources.at(i)) {
					auto& atomic_neuron =
					    m_config.neuron_block
					        .atomic_neurons[halco::hicann_dls::vx::v3::AtomicNeuronOnDLS(
					            neuron_view.columns.at(i), neuron_view.row)];
					atomic_neuron.readout.source = *neuron_view.neuron_readout_sources.at(i);
					atomic_neuron.readout.enable_amplifier = true;
				}
			}
		}
	}
	// store on-PPU handles for later PPU source code generation
	m_plasticity_rules.push_back({vertex, data, std::move(synapses), std::move(neurons)});
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::SynapseDriver const& data)
{
	auto& synapse_driver_config =
	    m_config.synapse_driver_blocks[data.get_coordinate().toSynapseDriverBlockOnDLS()]
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
    signal_flow::vertex::PADIBus const& data)
{
	auto const bus = data.get_coordinate();
	auto& config =
	    m_config.synapse_driver_blocks[bus.toPADIBusBlockOnDLS().toSynapseDriverBlockOnDLS()]
	        .padi_bus;
	auto enable_config = config.get_enable_spl1();
	enable_config[bus.toPADIBusOnPADIBusBlock()] = true;
	config.set_enable_spl1(enable_config);
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::CrossbarNode const& data)
{
	m_config.crossbar.nodes[data.get_coordinate()] = data.get_config();
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::BackgroundSpikeSource const& data)
{
	m_config.background_spike_sources[data.get_coordinate()] = data.get_config();
}

template <>
void ExecutionInstanceConfigVisitor::process(
    signal_flow::Graph::vertex_descriptor const vertex, signal_flow::vertex::NeuronView const& data)
{
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	size_t i = 0;
	auto const& configs = data.get_configs();
	auto& neurons = m_config.neuron_block.atomic_neurons;
	for (auto const column : data.get_columns()) {
		auto const& config = configs.at(i);
		auto const an = AtomicNeuronOnDLS(column, data.get_row());
		auto const neuron_reset = an.toNeuronResetOnDLS();
		m_enabled_neuron_resets[neuron_reset] = config.enable_reset;
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
			m_config.neuron_block.current_rows[data.get_row().toColumnCurrentRowOnDLS()]
			    .values[column]
			    .set_enable_synaptic_current_excitatory(true);
			m_config.neuron_block.current_rows[data.get_row().toColumnCurrentRowOnDLS()]
			    .values[column]
			    .set_enable_synaptic_current_inhibitory(true);
		}
	}
}

std::tuple<
    ExecutionInstanceConfigVisitor::PpuUsage,
    halco::common::typed_array<bool, halco::hicann_dls::vx::v3::NeuronResetOnDLS>>
ExecutionInstanceConfigVisitor::operator()()
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceConfigVisitor");
	auto const execution_instance_vertex =
	    m_graph.get_execution_instance_map().right.at(m_execution_instance);
	for (auto const p : boost::make_iterator_range(
	         m_graph.get_vertex_descriptor_map().right.equal_range(execution_instance_vertex))) {
		auto const vertex = p.second;
		std::visit(
		    [&](auto const& value) {
			    hate::Timer timer;
			    process(vertex, value);
			    LOG4CXX_TRACE(
			        logger, "process(): Preprocessed "
			                    << hate::name<hate::remove_all_qualifiers_t<decltype(value)>>()
			                    << " in " << timer.print() << ".");
		    },
		    m_graph.get_vertex_property(vertex));
	}

	ExecutionInstanceConfigVisitor::PpuUsage ppu_usage;
	ppu_usage.has_periodic_cadc_readout = m_has_periodic_cadc_readout;
	ppu_usage.has_cadc_readout = m_has_cadc_readout;
	ppu_usage.plasticity_rules = std::move(m_plasticity_rules);
	return {std::move(ppu_usage), std::move(m_enabled_neuron_resets)};
}

} // namespace grenade::vx::execution::detail
