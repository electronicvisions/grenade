#include "grenade/vx/execution_instance_config_builder.h"

#include "grenade/vx/execution_instance.h"
#include "grenade/vx/ppu.h"
#include "haldls/vx/v2/padi.h"
#include "hate/timer.h"
#include "hate/type_index.h"
#include "hate/type_traits.h"
#include "lola/vx/ppu.h"
#include <vector>
#include <boost/range/combine.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx {

ExecutionInstanceConfigBuilder::ExecutionInstanceConfigBuilder(
    Graph const& graph,
    coordinate::ExecutionInstance const& execution_instance,
    ChipConfig const& chip_config) :
    m_graph(graph), m_execution_instance(execution_instance), m_config(chip_config)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx;
	m_used_padi_busses.fill(false);

	/** Silence everything which is not set in the graph. */
	for (auto& node : m_config.crossbar_nodes) {
		node = haldls::vx::v2::CrossbarNode::drop_all;
	}
	for (auto const& hemisphere : iter_all<HemisphereOnDLS>()) {
		for (auto const drv : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
			m_config.hemispheres[hemisphere].synapse_driver_block[drv].set_row_mode_top(
			    haldls::vx::v2::SynapseDriverConfig::RowMode::disabled);
			m_config.hemispheres[hemisphere].synapse_driver_block[drv].set_row_mode_bottom(
			    haldls::vx::v2::SynapseDriverConfig::RowMode::disabled);
		}
	}
	{
		auto const new_matrix = std::make_unique<lola::vx::v2::SynapseMatrix>();
		for (auto const& hemisphere : iter_all<HemisphereOnDLS>()) {
			m_config.hemispheres[hemisphere].synapse_matrix = *new_matrix;
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
void ExecutionInstanceConfigBuilder::process(
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
void ExecutionInstanceConfigBuilder::process(
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
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const /* vertex */, vertex::CrossbarNode const& data)
{
	m_config.crossbar_nodes[data.get_coordinate()] = data.get_config();
}

template <>
void ExecutionInstanceConfigBuilder::process(
    Graph::vertex_descriptor const /*vertex*/, vertex::NeuronView const& data)
{
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
	size_t i = 0;
	auto const& enable_resets = data.get_enable_resets();
	for (auto const column : data.get_columns()) {
		auto const neuron_reset = AtomicNeuronOnDLS(column, data.get_row()).toNeuronResetOnDLS();
		m_enabled_neuron_resets[neuron_reset] = enable_resets.at(i);
		i++;
	}
	// TODO: once we have neuron configuration, it should be placed here
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

std::tuple<
    stadls::vx::v2::PlaybackProgramBuilder,
    std::optional<lola::vx::v2::PPUElfFile::symbols_type>>
ExecutionInstanceConfigBuilder::generate(bool const enable_ppu)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;
	using namespace stadls::vx::v2;
	using namespace lola::vx::v2;

	PlaybackProgramBuilder builder;
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

	std::optional<PPUElfFile::symbols_type> ppu_symbols;
	if (enable_ppu) {
		PPUMemoryBlock ppu_program;
		PPUMemoryBlockOnPPU ppu_neuron_reset_mask_coord;
		{
			PPUElfFile ppu_elf_file(get_program_path(ppu_program_name));
			ppu_program = ppu_elf_file.read_program();
			ppu_symbols = ppu_elf_file.read_symbols();
			ppu_neuron_reset_mask_coord = ppu_symbols->at("neuron_reset_mask").coordinate;
		}

		for (auto const ppu : iter_all<PPUOnDLS>()) {
			// bring PPU in reset state
			PPUControlRegister ctrl;
			ctrl.set_inhibit_reset(false);
			builder.write(ppu.toPPUControlRegisterOnDLS(), ctrl);

			// write PPU program
			PPUMemoryBlockOnDLS coord(
			    PPUMemoryBlockOnPPU(
			        PPUMemoryWordOnPPU(), PPUMemoryWordOnPPU(ppu_program.get_words().size() - 1)),
			    ppu);
			builder.write(coord, ppu_program);

			// bring PPU in running state
			ctrl.set_inhibit_reset(true);
			builder.write(ppu.toPPUControlRegisterOnDLS(), ctrl);

			// write neuron reset mask
			std::vector<int8_t> values(NeuronColumnOnDLS::size);
			for (auto const col : iter_all<NeuronColumnOnDLS>()) {
				values[col] = m_enabled_neuron_resets[AtomicNeuronOnDLS(col, ppu.toNeuronRowOnDLS())
				                                          .toNeuronResetOnDLS()];
			}
			auto const neuron_reset_mask = to_vector_unit_row(values);
			builder.write(PPUMemoryBlockOnDLS(ppu_neuron_reset_mask_coord, ppu), neuron_reset_mask);
		}
	}

	return {std::move(builder), ppu_symbols};
}

} // namespace grenade::vx