#include "grenade/vx/execution_instance_config_builder.h"

#include "grenade/vx/execution_instance.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/status.h"
#include "grenade/vx/ppu_program_generator.h"
#include "haldls/vx/v2/barrier.h"
#include "haldls/vx/v2/padi.h"
#include "hate/timer.h"
#include "hate/type_index.h"
#include "hate/type_traits.h"
#include "lola/vx/ppu.h"
#include <filesystem>
#include <vector>
#include <boost/range/combine.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx {

ExecutionInstanceConfigBuilder::ExecutionInstanceConfigBuilder(
    Graph const& graph,
    coordinate::ExecutionInstance const& execution_instance,
    lola::vx::v2::Chip const& chip_config) :
    m_graph(graph), m_execution_instance(execution_instance), m_config(chip_config)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
	m_requires_ppu = false;
	m_used_madc = false;

	/** Silence everything which is not set in the graph. */
	for (auto& node : m_config.crossbar.nodes) {
		node = haldls::vx::v2::CrossbarNode::drop_all;
	}
	for (auto const& block : iter_all<SynapseDriverBlockOnDLS>()) {
		for (auto const drv : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
			m_config.synapse_driver_blocks[block].synapse_drivers[drv].set_row_mode_top(
			    haldls::vx::v2::SynapseDriverConfig::RowMode::disabled);
			m_config.synapse_driver_blocks[block].synapse_drivers[drv].set_row_mode_bottom(
			    haldls::vx::v2::SynapseDriverConfig::RowMode::disabled);
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
				    lola::vx::v2::SynapseMatrix::Weight(0));
				m_config.synapse_blocks[block].matrix.labels[row].fill(
				    lola::vx::v2::SynapseMatrix::Label(0));
			}
		}
	}
}

template <typename T>
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const /* vertex */, T const& /* data */)
{
	// Spezialize for types which change static configuration
}

template <>
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::CADCMembraneReadoutView const& /* data */)
{
	m_requires_ppu = true;
}

template <>
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const, vertex::NeuronEventOutputView const& data)
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
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::MADCReadoutView const& data)
{
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;

	// MADCReadoutView inputs size equals 1
	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);

	// Determine readout chain configuration
	auto& smux = m_config.readout_chain.input_mux[SourceMultiplexerOnReadoutSourceSelection()];
	bool const is_odd = data.get_coord().toNeuronColumnOnDLS() % 2;
	auto neuron_even = smux.get_neuron_even();
	neuron_even[data.get_coord().toNeuronRowOnDLS().toHemisphereOnDLS()] = !is_odd;
	smux.set_neuron_even(neuron_even);
	auto neuron_odd = smux.get_neuron_odd();
	neuron_odd[data.get_coord().toNeuronRowOnDLS().toHemisphereOnDLS()] = is_odd;
	smux.set_neuron_odd(neuron_odd);
	m_config.readout_chain.buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection()].enable = true;
	// Configure neuron
	m_config.neuron_block.atomic_neurons[data.get_coord()].readout.source = data.get_config();
	m_config.neuron_block.atomic_neurons[data.get_coord()].readout.enable_amplifier = true;
	m_config.neuron_block.atomic_neurons[data.get_coord()].readout.enable_buffered_access = true;
	// Configure analog parameters
	// TODO: should come from calibration
	for (auto const smux : halco::common::iter_all<SourceMultiplexerOnReadoutSourceSelection>()) {
		m_config.readout_chain.buffer_to_pad[smux].amp_i_bias = CapMemCell::Value(0);
	}
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
void ExecutionInstanceConfigBuilder::process(
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
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::SynapseArrayViewSparse const& data)
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
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::PlasticityRule const& data)
{
	auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
	std::vector<std::pair<halco::hicann_dls::vx::v2::SynramOnDLS, ppu::SynapseArrayViewHandle>>
	    synapses;
	for (auto const in_edge : boost::make_iterator_range(in_edges)) {
		auto const& view = std::get<vertex::SynapseArrayViewSparse>(
		    m_graph.get_vertex_property(boost::source(in_edge, m_graph.get_graph())));
		synapses.push_back({view.get_synram(), view.toSynapseArrayViewHandle()});
	}
	m_plasticity_rules.push_back({data, std::move(synapses)});
	m_requires_ppu = true;
}

template <>
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::SynapseDriver const& data)
{
	auto& synapse_driver_config =
	    m_config.synapse_driver_blocks[data.get_coordinate().toSynapseDriverBlockOnDLS()]
	        .synapse_drivers[data.get_coordinate().toSynapseDriverOnSynapseDriverBlock()];
	synapse_driver_config.set_row_mode_top(
	    data.get_config().row_modes[halco::hicann_dls::vx::v2::SynapseRowOnSynapseDriver::top]);
	synapse_driver_config.set_row_mode_bottom(
	    data.get_config().row_modes[halco::hicann_dls::vx::v2::SynapseRowOnSynapseDriver::bottom]);
	synapse_driver_config.set_row_address_compare_mask(data.get_config().row_address_compare_mask);
	synapse_driver_config.set_enable_address_out(data.get_config().enable_address_out);
	synapse_driver_config.set_enable_receiver(true);
}

template <>
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::PADIBus const& data)
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
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::CrossbarNode const& data)
{
	m_config.crossbar.nodes[data.get_coordinate()] = data.get_config();
}

template <>
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::BackgroundSpikeSource const& data)
{
	m_config.background_spike_sources[data.get_coordinate()] = data.get_config();
}

template <>
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::NeuronView const& data)
{
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
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
			if (std::holds_alternative<vertex::SynapseArrayView>(
			        m_graph.get_vertex_property(source))) {
				auto const& synapses =
				    std::get<vertex::SynapseArrayView>(m_graph.get_vertex_property(source));
				for (auto const& column : synapses.get_columns()) {
					incoming_synapse_columns.insert(column);
				}
			} else {
				auto const& synapses =
				    std::get<vertex::SynapseArrayViewSparse>(m_graph.get_vertex_property(source));
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

void ExecutionInstanceConfigBuilder::pre_process()
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceConfigBuilder");
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
}

std::tuple<lola::vx::v2::Chip, std::optional<lola::vx::v2::PPUElfFile::symbols_type>>
ExecutionInstanceConfigBuilder::generate()
{
	static log4cxx::Logger* const logger =
	    log4cxx::Logger::getLogger("grenade.ExecutionInstanceConfigBuilder.generate()");

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
	using namespace stadls::vx::v2;
	using namespace lola::vx::v2;

	hate::Timer ppu_timer;
	std::optional<PPUElfFile::symbols_type> ppu_symbols;
	if (m_requires_ppu) {
		PPUMemoryBlock ppu_program;
		PPUMemoryBlockOnPPU ppu_neuron_reset_mask_coord;
		PPUMemoryWordOnPPU ppu_location_coord;
		PPUMemoryBlockOnPPU ppu_status_coord;
		{
			TemporaryDirectory temporary("grenade-ppu-XXXXXX");
			Compiler compiler;
			PPUProgramGenerator ppu_program_generator;
			for (auto const& [rule, synapses] : m_plasticity_rules) {
				ppu_program_generator.add(rule, synapses);
			}
			std::vector<std::string> sources_str = ppu_program_generator.done();
			std::vector<std::string> sources;
			size_t i = 0;
			for (auto const& source : sources_str) {
				auto const path = temporary.get_path() / ("source_" + std::to_string(i) + ".cpp");
				{
					std::ofstream fs(path);
					fs << source;
				}
				i++;
				sources.push_back(path);
			}
			sources.push_back(get_program_base_source());
			compiler.options_before_source.push_back("-DLIBNUX_TIME_RESOLUTION_SHIFT=0");
			{
				auto& program_cache = get_program_cache();
				std::lock_guard lock(program_cache.data_mutex);
				ProgramCache::Source source;
				source.options_before_source = compiler.options_before_source;
				source.options_after_source = compiler.options_after_source;
				source.source_codes = sources_str;
				auto const sha1 = source.sha1();
				if (program_cache.data.contains(sha1)) {
					auto const& program = program_cache.data.at(sha1);
					ppu_program = program.second;
					ppu_symbols = program.first;
				} else {
					auto const program = compiler.compile(sources);
					ppu_program = program.second;
					ppu_symbols = program.first;
					program_cache.data[sha1] = program;
				}
			}
			ppu_neuron_reset_mask_coord = ppu_symbols->at("neuron_reset_mask").coordinate;
			ppu_location_coord = ppu_symbols->at("ppu").coordinate.toMin();
			ppu_status_coord = ppu_symbols->at("status").coordinate;
		}
		LOG4CXX_TRACE(logger, "Generated PPU program in " << ppu_timer.print() << ".");

		for (auto const ppu : iter_all<PPUOnDLS>()) {
			// zero-initialize all symbols (esp. bss)
			assert(ppu_symbols);
			for (auto const& [_, symbol] : *ppu_symbols) {
				PPUMemoryBlock config(symbol.coordinate.toPPUMemoryBlockSize());
				m_config.ppu_memory[ppu.toPPUMemoryOnDLS()].set_block(symbol.coordinate, config);
			}

			// set PPU program
			m_config.ppu_memory[ppu.toPPUMemoryOnDLS()].set_block(
			    PPUMemoryBlockOnPPU(
			        PPUMemoryWordOnPPU(), PPUMemoryWordOnPPU(ppu_program.get_words().size() - 1)),
			    ppu_program);

			// set neuron reset mask
			halco::common::typed_array<int8_t, NeuronColumnOnDLS> values;
			for (auto const col : iter_all<NeuronColumnOnDLS>()) {
				values[col] = m_enabled_neuron_resets[AtomicNeuronOnDLS(col, ppu.toNeuronRowOnDLS())
				                                          .toNeuronResetOnDLS()];
			}
			auto const neuron_reset_mask = to_vector_unit_row(values);
			m_config.ppu_memory[ppu.toPPUMemoryOnDLS()].set_block(
			    ppu_neuron_reset_mask_coord, neuron_reset_mask);
			m_config.ppu_memory[ppu.toPPUMemoryOnDLS()].set_word(
			    ppu_location_coord, PPUMemoryWord::Value(ppu.value()));
		}
	}
	return {m_config, ppu_symbols};
}

} // namespace grenade::vx
