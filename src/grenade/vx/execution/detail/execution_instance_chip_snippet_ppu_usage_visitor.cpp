#include "grenade/vx/execution/detail/execution_instance_chip_snippet_ppu_usage_visitor.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/execution/detail/ppu_program_generator.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/pad_readout.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"
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
    grenade::common::LinkedTopology const& topology,
    grenade::common::VertexOnTopology const& execution_instance_vertex_descriptor,
    grenade::common::InputData const& input_data,
    common::ChipOnConnection const& chip_on_connection,
    size_t snippet_index) :
    m_topology(topology),
    m_execution_instance_vertex_descriptor(execution_instance_vertex_descriptor),
    m_input_data(input_data),
    m_chip_on_connection(chip_on_connection),
    m_snippet_index(snippet_index),
    m_logger(log4cxx::Logger::getLogger("grenade.ExecutionInstanceChipSnippetPPUUsageVisitor"))
{
	register_process<signal_flow::vertex::CADCMembraneReadoutView>();
	register_process<signal_flow::vertex::NeuronView>();
	register_process<signal_flow::vertex::PlasticityRule>();
}

template <typename T>
void ExecutionInstanceChipSnippetPPUUsageVisitor::register_process()
{
	m_visitor.emplace(
	    std::type_index(typeid(T)), [&](grenade::common::VertexOnTopology const& vertex_descriptor,
	                                    grenade::common::Vertex const& vertex, Result& result) {
		    hate::Timer timer;
		    process(vertex_descriptor, static_cast<T const&>(vertex), result);
		    LOG4CXX_TRACE(
		        m_logger, "process(): Processed " << hate::name<hate::remove_all_qualifiers_t<T>>()
		                                          << " in " << timer.print() << ".");
	    });
}

template <typename T>
void ExecutionInstanceChipSnippetPPUUsageVisitor::process(
    grenade::common::VertexOnTopology const /* vertex */,
    T const& /* data */,
    Result& /* result */) const
{
	// Spezialize for types which change static configuration
}

template <>
void ExecutionInstanceChipSnippetPPUUsageVisitor::process(
    grenade::common::VertexOnTopology const /* vertex */,
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
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::PlasticityRule const& data,
    Result& result) const
{
	// convert the plasticity rule input vertices to their respective on-PPU handles to transfer
	// onto the PPUs
	std::vector<std::pair<halco::hicann_dls::vx::v3::SynramOnDLS, ppu::SynapseArrayViewHandle>>
	    synapses(data.get_synapse_view_shapes().size());
	std::vector<std::pair<halco::hicann_dls::vx::v3::NeuronRowOnDLS, ppu::NeuronViewHandle>>
	    neurons(data.get_neuron_view_shapes().size());
	for (auto const in_edge : m_topology.get_reference().in_edges(vertex)) {
		// extract on-PPU handles from source vertex properties
		auto const& in_vertex_property =
		    m_topology.get_reference().get(m_topology.get_reference().source(in_edge));
		if (auto const synapses_sparse =
		        dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse const*>(
		            &in_vertex_property);
		    synapses_sparse) {
			synapses.at(m_topology.get_reference().get(in_edge).port_on_target) = {
			    synapses_sparse->get_synram(), synapses_sparse->toSynapseArrayViewHandle()};
		} else if (auto const neurons_ptr =
		               dynamic_cast<signal_flow::vertex::NeuronView const*>(&in_vertex_property);
		           neurons_ptr) {
			neurons.at(
			    m_topology.get_reference().get(in_edge).port_on_target -
			    data.get_synapse_view_shapes().size()) = {
			    neurons_ptr->row, neurons_ptr->toNeuronViewHandle()};
		}
	}
	// store on-PPU handles for later PPU source code generation
	result.plasticity_rules.push_back(
	    {vertex, data, std::move(synapses), std::move(neurons), m_snippet_index});
}

template <>
void ExecutionInstanceChipSnippetPPUUsageVisitor::process(
    grenade::common::VertexOnTopology const vertex,
    signal_flow::vertex::NeuronView const& data,
    Result& result) const
{
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	auto const& parameterization =
	    dynamic_cast<signal_flow::vertex::NeuronView::Parameterization const&>(
	        m_input_data.ports.get(grenade::common::PortOnTopology{vertex, 1}));
	size_t i = 0;
	auto const& configs = parameterization.configs;
	for (auto const column : data.get_columns()) {
		auto const& config = configs.at(i);
		auto const an = AtomicNeuronOnDLS(column, data.row);
		auto const neuron_reset = an.toNeuronResetOnDLS();
		result.enabled_neuron_resets[neuron_reset] = config.enable_reset;
		i++;
	}
}

ExecutionInstanceChipSnippetPPUUsageVisitor::Result
ExecutionInstanceChipSnippetPPUUsageVisitor::operator()() const
{
	Result result;
	for (auto const inter_topology_hyper_edge_descriptor :
	     m_topology.inter_graph_hyper_edges_by_linked(m_execution_instance_vertex_descriptor)) {
		for (auto const& vertex_descriptor :
		     m_topology.references(inter_topology_hyper_edge_descriptor)) {
			auto const& vertex = m_topology.get_reference().get(vertex_descriptor);
			if (m_visitor.contains(std::type_index(typeid(vertex)))) {
				m_visitor.at(std::type_index(typeid(vertex)))(vertex_descriptor, vertex, result);
			}
		}
	}

	return result;
}

} // namespace grenade::vx::execution::detail
