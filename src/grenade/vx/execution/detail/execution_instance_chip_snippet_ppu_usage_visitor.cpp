#include "grenade/vx/execution/detail/execution_instance_chip_snippet_ppu_usage_visitor.h"

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

ExecutionInstanceChipSnippetPPUUsageVisitor::Result::Result() :
    has_periodic_cadc_readout(false),
    has_periodic_cadc_readout_on_dram(false),
    has_cadc_readout(false),
    plasticity_rules(),
    enabled_neuron_resets()
{
	enabled_neuron_resets.fill(false);
}

ExecutionInstanceChipSnippetPPUUsageVisitor::Result::Result(
    ExecutionInstanceChipSnippetPPUUsageVisitor::Result&& other) :
    has_periodic_cadc_readout(other.has_periodic_cadc_readout),
    has_periodic_cadc_readout_on_dram(other.has_periodic_cadc_readout_on_dram),
    has_cadc_readout(other.has_cadc_readout),
    plasticity_rules(std::move(other.plasticity_rules)),
    enabled_neuron_resets(std::move(other.enabled_neuron_resets))
{
}

ExecutionInstanceChipSnippetPPUUsageVisitor::Result&
ExecutionInstanceChipSnippetPPUUsageVisitor::Result::operator=(
    ExecutionInstanceChipSnippetPPUUsageVisitor::Result&& other)
{
	has_periodic_cadc_readout = other.has_periodic_cadc_readout;
	has_periodic_cadc_readout_on_dram = other.has_periodic_cadc_readout_on_dram;
	has_cadc_readout = other.has_cadc_readout;
	plasticity_rules = std::move(other.plasticity_rules);
	enabled_neuron_resets = std::move(other.enabled_neuron_resets);
	return *this;
}

ExecutionInstanceChipSnippetPPUUsageVisitor::Result&
ExecutionInstanceChipSnippetPPUUsageVisitor::Result::operator+=(
    ExecutionInstanceChipSnippetPPUUsageVisitor::Result&& other)
{
	has_periodic_cadc_readout = has_periodic_cadc_readout || other.has_periodic_cadc_readout;
	has_periodic_cadc_readout_on_dram =
	    has_periodic_cadc_readout_on_dram || other.has_periodic_cadc_readout_on_dram;
	has_cadc_readout = has_cadc_readout || other.has_cadc_readout;
	plasticity_rules.insert(
	    plasticity_rules.begin(), std::make_move_iterator(other.plasticity_rules.begin()),
	    std::make_move_iterator(other.plasticity_rules.end()));
	for (auto const coord :
	     halco::common::iter_all<halco::hicann_dls::vx::v3::NeuronResetOnDLS>()) {
		enabled_neuron_resets[coord] |= other.enabled_neuron_resets[coord];
	}
	return *this;
}

ExecutionInstanceChipSnippetPPUUsageVisitor::ExecutionInstanceChipSnippetPPUUsageVisitor(
    signal_flow::Graph const& graph,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::ExecutionInstanceID const& execution_instance,
    size_t snippet_index) :
    m_graph(graph),
    m_chip_on_connection(chip_on_connection),
    m_execution_instance(execution_instance),
    m_snippet_index(snippet_index)
{
}

template <typename T>
void ExecutionInstanceChipSnippetPPUUsageVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    T const& /* data */,
    Result& /* result */) const
{
	// Spezialize for types which change static configuration
}

template <>
void ExecutionInstanceChipSnippetPPUUsageVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::CADCMembraneReadoutView const& data,
    Result& result) const
{
	result.has_periodic_cadc_readout =
	    data.get_mode() == signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic;
	result.has_periodic_cadc_readout_on_dram =
	    data.get_mode() == signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic_on_dram;
	result.has_cadc_readout = true;
}

template <>
void ExecutionInstanceChipSnippetPPUUsageVisitor::process(
    signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::PlasticityRule const& data,
    Result& result) const
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
	}
	// store on-PPU handles for later PPU source code generation
	result.plasticity_rules.push_back(
	    {vertex, data, std::move(synapses), std::move(neurons), m_snippet_index});
}

template <>
void ExecutionInstanceChipSnippetPPUUsageVisitor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::NeuronView const& data,
    Result& result) const
{
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	size_t i = 0;
	auto const& configs = data.get_configs();
	for (auto const column : data.get_columns()) {
		auto const& config = configs.at(i);
		auto const an = AtomicNeuronOnDLS(column, data.get_row());
		auto const neuron_reset = an.toNeuronResetOnDLS();
		result.enabled_neuron_resets[neuron_reset] = config.enable_reset;
		i++;
	}
}

ExecutionInstanceChipSnippetPPUUsageVisitor::Result
ExecutionInstanceChipSnippetPPUUsageVisitor::operator()() const
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceChipSnippetPPUUsageVisitor");
	Result result;
	auto const execution_instance_vertex =
	    m_graph.get_execution_instance_map().right.at(m_execution_instance);
	for (auto const p : boost::make_iterator_range(
	         m_graph.get_vertex_descriptor_map().right.equal_range(execution_instance_vertex))) {
		auto const vertex = p.second;
		std::visit(
		    [&](auto const& value) {
			    hate::Timer timer;
			    process(vertex, value, result);
			    typedef hate::remove_all_qualifiers_t<decltype(value)> vertex_type;
			    if constexpr (std::is_base_of_v<vertex_type, common::EntityOnChip>) {
				    if (value.chip_on_connection == m_chip_on_connection) {
					    process(vertex, value);
				    }
			    }
			    LOG4CXX_TRACE(
			        logger, "process(): Processed "
			                    << hate::name<hate::remove_all_qualifiers_t<decltype(value)>>()
			                    << " in " << timer.print() << ".");
		    },
		    m_graph.get_vertex_property(vertex));
	}

	return result;
}

} // namespace grenade::vx::execution::detail
