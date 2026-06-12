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
    ResourceEstimator const& resource_estimator,
    SystemResources const& system_resources,
    std::shared_ptr<LinkedTopology> topology) :
    TopologyRewrite(std::move(topology)),
    m_resource_estimator(resource_estimator.copy()),
    m_system_resources(system_resources)
{
}

void PopulationTopologyRewrite::operator()() const
{
	if (!m_resource_estimator) {
		throw std::logic_error("Unexpected access to moved-from object.");
	}
	if (m_system_resources.empty()) {
		throw std::runtime_error("PopulationTopologyRewrite requires non-empty system resources.");
	}
	// copy vertex descriptors because they are modified in the loop (by replacing populations)
	std::vector<VertexOnTopology> all_population_vertices;
	for (auto const& vertex_descriptor : get_topology().vertices()) {
		if (auto const ptr =
		        dynamic_cast<Population const*>(&(get_topology().get(vertex_descriptor)));
		    ptr) {
			all_population_vertices.push_back(vertex_descriptor);
		}
	}
	auto current_system_resources_it = m_system_resources.begin();
	for (auto const vertex_descriptor : all_population_vertices) {
		auto const& population =
		    dynamic_cast<Population const&>(get_topology().get(vertex_descriptor));

		// get reference population vertex descriptor
		std::unordered_set<typename LinkedTopology::ReferenceGraph::VertexDescriptor>
		    old_references;
		for (auto const& link :
		     get_topology().inter_graph_hyper_edges_by_linked(vertex_descriptor)) {
			auto const& references = get_topology().references(link);
			assert(references.size() == 1);
			old_references.insert(references.at(0));
		}
		assert(old_references.size() == 1);
		auto const& old_reference = *old_references.begin();

		// storage for slice indices and associated connections
		std::set<size_t> slice_indices;
		std::vector<ConnectionOnExecutor> connections_on_executor;

		auto const resource_estimation = m_resource_estimator->operator()(vertex_descriptor);
		assert(resource_estimation);

		// We iterate trying to slice the remaining sequence of cells on the population until the
		// currently sliced sequence resource requirements are larger than the system resources.
		// Then we remember this slice, select the next connection on the executor (and its system
		// resources) and start over with the remaining sequence.
		auto const& population_shape = population.get_shape();
		std::optional<size_t> largest_performable_slice = std::nullopt;
		size_t last_performed_slice = 0;
		auto remaining_slice_sequence = population_shape.copy();
		for (size_t current_slice = 1; current_slice <= population_shape.size(); ++current_slice) {
			auto const current_slice_sequence = std::move(
			    remaining_slice_sequence->slice({current_slice - last_performed_slice}).at(0));
			assert(current_system_resources_it != m_system_resources.end());
			if (!resource_estimation->subsequence(*current_slice_sequence)
			         ->any_scalar_greater(*current_system_resources_it->second)) {
				largest_performable_slice = current_slice;
			} else {
				if (!largest_performable_slice) {
					throw std::runtime_error(
					    "No slice found for population which is smaller than system resources.");
				} else {
					slice_indices.insert(*largest_performable_slice);
					connections_on_executor.push_back(current_system_resources_it->first);

					remaining_slice_sequence =
					    std::move(remaining_slice_sequence
					                  ->slice({*largest_performable_slice - last_performed_slice})
					                  .at(1));
					last_performed_slice = *largest_performable_slice;

					largest_performable_slice = std::nullopt;

					current_slice -= 1;

					current_system_resources_it++;
					if (current_system_resources_it == m_system_resources.end()) {
						current_system_resources_it = m_system_resources.begin();
					}
				}
			}
		}
		connections_on_executor.push_back(current_system_resources_it->first);

		assert(slice_indices.size() + 1 == connections_on_executor.size());
		auto const new_population_shapes = population_shape.slice(slice_indices);

		// create new populations
		std::vector<Population> new_populations;
		for (size_t i = 0; auto const& new_population_shape : new_population_shapes) {
			assert(new_population_shape);
			auto new_population_parameter_space = population.get_parameter_space().get_section(
			    *CuboidMultiIndexSequence({population.get_shape().size()})
			         .related_sequence_subset_restriction(
			             population.get_shape(), *new_population_shape));
			assert(new_population_parameter_space);
			if (population.get_execution_instance_on_executor()) {
				throw std::runtime_error(
				    "Partitioning population, for which the execution instance "
				    "on executor is constrained, is not supported.");
			}
			new_populations.emplace_back(Population(
			    population.get_cell(), *new_population_shape, *new_population_parameter_space,
			    population.get_time_domain(),
			    ExecutionInstanceOnExecutor(ExecutionInstanceID(), connections_on_executor.at(i))));
			i++;
		}

		// add new populations to topology
		std::vector<VertexOnTopology> new_vertex_descriptors;
		for (auto const& new_population : new_populations) {
			auto const new_population_descriptor = get_topology().add_vertex(new_population);
			get_topology().add_inter_graph_hyper_edge(
			    {new_population_descriptor}, {old_reference}, PopulationInterTopologyHyperEdge());
			new_vertex_descriptors.push_back(new_population_descriptor);
		}

		// create new in-edges
		std::vector<std::tuple<VertexOnTopology, VertexOnTopology, Edge>> new_in_edges;
		for (auto const& new_vertex_descriptor : new_vertex_descriptors) {
			for (auto const in_edge_descriptor : get_topology().in_edges(vertex_descriptor)) {
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
			for (auto const out_edge_descriptor : get_topology().out_edges(vertex_descriptor)) {
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
				auto const channels_on_source =
				    out_edge.get_channels_on_source().subset_restriction(
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

		get_topology().clear_vertex(vertex_descriptor);

		// add new in-edges
		for (auto const& [source, target, edge] : new_in_edges) {
			get_topology().add_edge(source, target, edge);
		}
		// add new out-edges
		for (auto const& [source, target, edge] : new_out_edges) {
			get_topology().add_edge(source, target, edge);
		}

		get_topology().remove_vertex(vertex_descriptor);
	}
}

} // namespace grenade::common
