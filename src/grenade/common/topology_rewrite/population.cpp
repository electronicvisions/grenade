#include "grenade/common/topology_rewrite/population.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/edge_on_topology.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/inter_topology_hyper_edge/population.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/population.h"
#include "hate/math.h"
#include <memory>

namespace grenade::common {

PopulationTopologyRewrite::PopulationTopologyRewrite(
    VertexOnTopology vertex_descriptor,
    LinkedTopology::ReferenceGraph::VertexDescriptor old_reference,
    std::vector<std::unique_ptr<MultiIndexSequence>> new_population_shapes,
    std::vector<ConnectionOnExecutor> connections_on_executor,
    std::shared_ptr<LinkedTopology> topology) :
    TopologyRewrite(std::move(topology)),
    m_vertex_descriptor(std::move(vertex_descriptor)),
    m_old_reference(std::move(old_reference)),
    m_new_population_shapes(std::move(new_population_shapes)),
    m_connections_on_executor(std::move(connections_on_executor))
{
}

void PopulationTopologyRewrite::operator()() const
{
	auto const& population =
	    dynamic_cast<Population const&>(get_topology().get(m_vertex_descriptor));
	auto const& execution_instance = population.get_execution_instance_on_executor();

	// create new populations
	std::vector<Population> new_populations;
	for (size_t i = 0; auto const& new_population_shape : m_new_population_shapes) {
		assert(new_population_shape);
		auto new_population_parameter_space = population.get_parameter_space().get_section(
		    *CuboidMultiIndexSequence({population.get_shape().size()})
		         .related_sequence_subset_restriction(
		             population.get_shape(), *new_population_shape));
		assert(new_population_parameter_space);

		ExecutionInstanceOnExecutor new_execution_instance(
		    ExecutionInstanceID(), m_connections_on_executor.at(i));
		if (execution_instance) {
			new_execution_instance = execution_instance.value();
		}

		new_populations.emplace_back(Population(
		    population.get_cell(), *new_population_shape, *new_population_parameter_space,
		    population.get_time_domain(),
		    ExecutionInstanceOnExecutor(
		        new_execution_instance.execution_instance_id, m_connections_on_executor.at(i))));
		i++;
	}

	// add new populations to topology
	std::vector<VertexOnTopology> new_vertex_descriptors;
	for (auto const& new_population : new_populations) {
		auto const new_population_descriptor = get_topology().add_vertex(new_population);
		get_topology().add_inter_graph_hyper_edge(
		    {new_population_descriptor}, {m_old_reference}, PopulationInterTopologyHyperEdge());
		new_vertex_descriptors.push_back(new_population_descriptor);
	}

	// create new in-edges
	std::vector<std::tuple<VertexOnTopology, VertexOnTopology, Edge>> new_in_edges;
	for (auto const& new_vertex_descriptor : new_vertex_descriptors) {
		for (auto const in_edge_descriptor : get_topology().in_edges(m_vertex_descriptor)) {
			auto const& in_edge = get_topology().get(in_edge_descriptor);
			// logic below only works for injective edge channels
			if (!in_edge.get_channels_on_source().is_injective() ||
			    !in_edge.get_channels_on_target().is_injective()) {
				throw std::runtime_error("PopulationTopologyRewrite edge rewrite only works "
				                         "for injective edge channels");
			}
			auto const& new_vertex =
			    dynamic_cast<Population const&>(get_topology().get(new_vertex_descriptor));
			auto const new_input_ports = new_vertex.get_input_ports();
			auto const channels_on_target =
			    new_input_ports.at(in_edge.port_on_target)
			        .get_channels()
			        .subset_restriction(in_edge.get_channels_on_target());
			assert(channels_on_target);
			auto const channels_on_source =
			    in_edge.get_channels_on_source().related_sequence_subset_restriction(
			        in_edge.get_channels_on_target(),
			        new_input_ports.at(in_edge.port_on_target).get_channels());
			assert(channels_on_source);
			if (channels_on_source->size() == 0) {
				continue;
			}
			new_in_edges.push_back(std::make_tuple(
			    get_topology().source(in_edge_descriptor), new_vertex_descriptor,
			    Edge(
			        *channels_on_source, *channels_on_target, in_edge.port_on_source,
			        in_edge.port_on_target)));
		}
	}

	// create new out-edges
	std::vector<std::tuple<VertexOnTopology, VertexOnTopology, Edge>> new_out_edges;
	for (auto const& new_vertex_descriptor : new_vertex_descriptors) {
		for (auto const out_edge_descriptor : get_topology().out_edges(m_vertex_descriptor)) {
			auto const& out_edge = get_topology().get(out_edge_descriptor);
			// logic below only works for injective edge channels
			if (!out_edge.get_channels_on_source().is_injective() ||
			    !out_edge.get_channels_on_target().is_injective()) {
				throw std::runtime_error("PopulationTopologyRewrite edge rewrite only works "
				                         "for injective edge channels");
			}
			auto const& new_vertex =
			    dynamic_cast<Population const&>(get_topology().get(new_vertex_descriptor));
			auto const new_output_ports = new_vertex.get_output_ports();
			auto const channels_on_source = out_edge.get_channels_on_source().subset_restriction(
			    new_output_ports.at(out_edge.port_on_source).get_channels());
			assert(channels_on_source);
			if (channels_on_source->size() == 0) {
				continue;
			}
			auto const channels_on_target =
			    out_edge.get_channels_on_target().related_sequence_subset_restriction(
			        out_edge.get_channels_on_source(), *channels_on_source);
			assert(channels_on_target);
			new_out_edges.push_back(std::make_tuple(
			    new_vertex_descriptor, get_topology().target(out_edge_descriptor),
			    Edge(
			        *channels_on_source, *channels_on_target, out_edge.port_on_source,
			        out_edge.port_on_target)));
		}
	}

	get_topology().clear_vertex(m_vertex_descriptor);

	// add new in-edges
	for (auto const& [source, target, edge] : new_in_edges) {
		get_topology().add_edge(source, target, edge);
	}
	// add new out-edges
	for (auto const& [source, target, edge] : new_out_edges) {
		get_topology().add_edge(source, target, edge);
	}

	get_topology().remove_vertex(m_vertex_descriptor);
}

} // namespace grenade::common
