#include "grenade/common/topology_rewrite/execution_instance.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/population.h"
#include "grenade/common/resource_estimator.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/topology_lazy_validity_checker.h"
#include "grenade/common/topology_rewrite/strong_component_invariant.h"
#include "grenade/common/vertex_on_topology.h"
#include <ctime>
#include <memory>
#include <stdexcept>
#include <boost/pending/disjoint_sets.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace grenade::common {

ExecutionInstanceTopologyRewrite::ExecutionInstanceTopologyRewrite(
    ResourceEstimator const& resource_estimator,
    SystemResources const& system_resources,
    std::shared_ptr<LinkedTopology> topology) :
    TopologyRewrite(std::move(topology)),
    m_resource_estimator(resource_estimator.copy()),
    m_system_resources(system_resources)
{
}

void ExecutionInstanceTopologyRewrite::operator()() const
{
	if (!m_resource_estimator) {
		throw std::logic_error("Unexpected access to moved-from object.");
	}
	// assign unique ID to each strongly connected component
	auto const strongly_connected_component_coloring = get_topology().strong_components();

	// assign unique ID to each vertex
	std::map<VertexOnTopology, ExecutionInstanceID> assigned_execution_instance_ids;
	for (auto const vertex_descriptor : get_topology().vertices()) {
		assigned_execution_instance_ids[vertex_descriptor] =
		    ExecutionInstanceID(strongly_connected_component_coloring.at(vertex_descriptor));
	}

	auto vertex_indices = get_topology().get_vertex_index_map();
	boost::associative_property_map<std::remove_const_t<decltype(vertex_indices)>>
	    vertex_indices_pmap(vertex_indices);

	typedef std::map<VertexOnTopology, VertexOnTopology> VertexParents; // parent per vertex
	VertexParents vertex_parents;
	boost::associative_property_map<VertexParents> vertex_parents_pmap(vertex_parents);

	boost::disjoint_sets<
	    boost::associative_property_map<std::remove_const_t<decltype(vertex_indices)>>,
	    boost::associative_property_map<VertexParents>>
	    disjoint_vertex_sets(vertex_indices_pmap, vertex_parents_pmap);
	for (auto const vertex_descriptor : get_topology().vertices()) {
		disjoint_vertex_sets.make_set(vertex_descriptor);
	}

	// merge IDs of vertices on strongly connected component
	std::map<size_t, std::set<VertexOnTopology>> strongly_connected_components;
	for (auto const& [vertex_descriptor, color] : strongly_connected_component_coloring) {
		strongly_connected_components[color].insert(vertex_descriptor);
	}
	for (auto const& [_, vertex_descriptors] : strongly_connected_components) {
		for (auto const& vertex_descriptor : vertex_descriptors) {
			if (vertex_descriptor == *vertex_descriptors.begin()) {
				continue;
			}
			disjoint_vertex_sets.union_set(*vertex_descriptors.begin(), vertex_descriptor);
		}
	}

	// merge IDs of vertices connected by edges which don't support execution instance transition.
	for (auto const edge_descriptor : get_topology().edges()) {
		auto const& edge = get_topology().get(edge_descriptor);
		auto const source_vertex_descriptor = get_topology().source(edge_descriptor);
		auto const target_vertex_descriptor = get_topology().target(edge_descriptor);
		auto const& source = dynamic_cast<PartitionedVertex const&>(
		    get_topology().get(get_topology().source(edge_descriptor)));
		auto const& target = dynamic_cast<PartitionedVertex const&>(
		    get_topology().get(get_topology().target(edge_descriptor)));
		auto const source_port = source.get_output_ports().at(edge.port_on_source);
		auto const target_port = target.get_input_ports().at(edge.port_on_target);
		if (source_port.execution_instance_transition_constraint ==
		        Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported ||
		    target_port.execution_instance_transition_constraint ==
		        Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported) {
			disjoint_vertex_sets.union_set(source_vertex_descriptor, target_vertex_descriptor);
		}
	}
	for (auto const vertex_descriptor : get_topology().vertices()) {
		auto const root_vertex_descriptor = disjoint_vertex_sets.find_set(vertex_descriptor);
		assigned_execution_instance_ids.at(vertex_descriptor) =
		    assigned_execution_instance_ids.at(root_vertex_descriptor);
	}

	// assign found execution instance ids to vertices in topology and restore connection on
	// executor information.
	// This yields a correct, but not space-optimized partitioning.
	{
		TopologyLazyValidityChecker lazy_validity_checker(get_topology());
		for (auto const& vertex_descriptor : get_topology().vertices()) {
			auto& partitioned_vertex =
			    dynamic_cast<PartitionedVertex&>(get_topology().get_mutable(vertex_descriptor));
			ExecutionInstanceOnExecutor execution_instance_on_executor;
			if (partitioned_vertex.get_execution_instance_on_executor()) {
				execution_instance_on_executor.connection_on_executor =
				    partitioned_vertex.get_execution_instance_on_executor()
				        .value()
				        .connection_on_executor;
			}
			execution_instance_on_executor.execution_instance_id =
			    assigned_execution_instance_ids.at(vertex_descriptor);
			partitioned_vertex.set_execution_instance_on_executor(execution_instance_on_executor);
		}
	}

	// Construct topology of strong component invariants and check that it is acyclic.
	// It should never by cyclic, but we topologically sort afterwards and need to make sure.
	std::shared_ptr<Topology const> const this_ptr(&get_topology(), [](Topology const*) {
	}); // non-owning shared ptr for local use in LinkedTopology
	auto const strong_component_invariant_topology = std::make_shared<LinkedTopology>(this_ptr);
	StrongComponentInvariantRewrite strong_component_invariant_rewrite(
	    strong_component_invariant_topology);
	strong_component_invariant_rewrite();
	if (!strong_component_invariant_topology->is_acyclic()) {
		throw std::logic_error(
		    "Execution instance assignment yields cyclic strong component invariant topology.");
	}

	// assign unique, topologically ordered, ID to each strong component invariant
	for (ExecutionInstanceID next_id;
	     auto const& vertex_descriptor : strong_component_invariant_topology->topological_sort()) {
		for (auto const& inter_topology_hyper_edge_descriptor :
		     strong_component_invariant_topology->inter_graph_hyper_edges_by_linked(
		         vertex_descriptor)) {
			for (auto const& reference_vertex_descriptor :
			     strong_component_invariant_topology->references(
			         inter_topology_hyper_edge_descriptor)) {
				assigned_execution_instance_ids.at(reference_vertex_descriptor) = next_id;
			}
		}
		next_id += ExecutionInstanceID(1);
	}

	// collect resources per ID
	std::map<
	    std::pair<std::optional<TimeDomainOnTopology>, ConnectionOnExecutor>,
	    std::map<ExecutionInstanceID, std::unique_ptr<ResourceEstimator::Resource>>>
	    used_resources;
	for (auto const& vertex_descriptor : get_topology().vertices()) {
		auto resource_estimation = m_resource_estimator->operator()(vertex_descriptor);
		// this should not happen, but depends on correct implementation of the resource
		// estimator.
		if (!resource_estimation) {
			continue;
		}
		auto const& partitioned_vertex =
		    dynamic_cast<PartitionedVertex const&>(get_topology().get(vertex_descriptor));
		if (!used_resources[std::pair{
		                        partitioned_vertex.get_time_domain(),
		                        partitioned_vertex.get_execution_instance_on_executor()
		                            .value()
		                            .connection_on_executor}]
		         .contains(assigned_execution_instance_ids.at(vertex_descriptor))) {
			used_resources
			    .at(std::pair{
			        partitioned_vertex.get_time_domain(),
			        partitioned_vertex.get_execution_instance_on_executor()
			            .value()
			            .connection_on_executor})
			    .emplace(
			        assigned_execution_instance_ids.at(vertex_descriptor),
			        std::move(resource_estimation));
		} else {
			assert(used_resources
			           .at(std::pair{
			               partitioned_vertex.get_time_domain(),
			               partitioned_vertex.get_execution_instance_on_executor()
			                   .value()
			                   .connection_on_executor})
			           .at(assigned_execution_instance_ids.at(vertex_descriptor)));
			*used_resources
			     .at(std::pair{
			         partitioned_vertex.get_time_domain(),
			         partitioned_vertex.get_execution_instance_on_executor()
			             .value()
			             .connection_on_executor})
			     .at(assigned_execution_instance_ids.at(vertex_descriptor)) += *resource_estimation;
		}
	}

	// merge IDs greedy-linear until resource requirements per execution instance would be exceeded
	// TODO: balanced number partitioning would solve this optimally (given the splits beforehand
	// were optimal)
	for (auto const& [td_conn, map] : used_resources) {
		auto const& [_, connection_on_executor] = td_conn;
		std::vector<std::set<ExecutionInstanceID>> execution_instance_collections(1);
		std::unique_ptr<ResourceEstimator::Resource> resource;
		for (auto const& [id, local_resource] : map) {
			assert(local_resource);
			if (local_resource->any_scalar_greater(
			        *m_system_resources.at(connection_on_executor))) {
				throw std::runtime_error("Assignment of execution instances unsuccessful.");
			}
			// add up resources
			if (resource) {
				*resource += *local_resource;
			} else {
				resource = local_resource->copy();
			}
			// if resources including local resource exceed system resources start a new collection,
			// keep the local resource and continue
			if (resource->any_scalar_greater(*m_system_resources.at(connection_on_executor))) {
				// add a new element in execution_instance_collections to insert next ids into
				if (!execution_instance_collections.back().empty()) {
					execution_instance_collections.push_back({});
				}
				resource = local_resource->copy();
			}
			// insert id into currently filled collection
			execution_instance_collections.back().insert(id);
		}

		// assign the first id to all vertices in each collection
		for (auto const& execution_instance_collection : execution_instance_collections) {
			assert(!execution_instance_collection.empty());
			for (auto const vertex_descriptor : get_topology().vertices()) {
				if (execution_instance_collection.contains(
				        assigned_execution_instance_ids.at(vertex_descriptor))) {
					assigned_execution_instance_ids.at(vertex_descriptor) =
					    *execution_instance_collection.begin();
				}
			}
		}
	}

	// remap to [0, num_execution_instances)
	{
		std::set<ExecutionInstanceID> all_execution_instances;
		for (auto const& [_, execution_instance] : assigned_execution_instance_ids) {
			all_execution_instances.insert(execution_instance);
		}
		for (auto& [_, execution_instance] : assigned_execution_instance_ids) {
			execution_instance = ExecutionInstanceID(std::distance(
			    all_execution_instances.begin(),
			    std::find(
			        all_execution_instances.begin(), all_execution_instances.end(),
			        execution_instance)));
		}
	}

	// assign found execution instance ids to vertices in topology and restore connection on
	// executor information
	{
		TopologyLazyValidityChecker lazy_validity_checker(get_topology());
		for (auto const vertex_descriptor : get_topology().vertices()) {
			auto& partitioned_vertex =
			    dynamic_cast<PartitionedVertex&>(get_topology().get_mutable(vertex_descriptor));
			ExecutionInstanceOnExecutor execution_instance_on_executor;
			if (partitioned_vertex.get_execution_instance_on_executor()) {
				execution_instance_on_executor.connection_on_executor =
				    partitioned_vertex.get_execution_instance_on_executor()
				        .value()
				        .connection_on_executor;
			}
			execution_instance_on_executor.execution_instance_id =
			    assigned_execution_instance_ids.at(vertex_descriptor);
			partitioned_vertex.set_execution_instance_on_executor(execution_instance_on_executor);
		}
	}
}

} // namespace grenade::common
