#include "grenade/common/topology_rewrite/populations_per_execution_instance.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/inter_topology_hyper_edge/population.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/population.h"
#include "grenade/common/topology_rewrite/population.h"
#include "hate/math.h"
#include <memory>

namespace grenade::common {

PopulationsPerExecutionInstanceTopologyRewrite::PopulationsPerExecutionInstanceTopologyRewrite(
    ResourceEstimator const& resource_estimator,
    SystemResources const& system_resources,
    std::shared_ptr<LinkedTopology> topology) :
    TopologyRewrite(std::move(topology)),
    m_resource_estimator(resource_estimator.copy()),
    m_system_resources(system_resources)
{
}

void PopulationsPerExecutionInstanceTopologyRewrite::operator()() const
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

		if (population.get_execution_instance_on_executor()) {
			throw std::runtime_error("Partitioning population, for which the execution instance "
			                         "on executor is constrained, is not supported.");
		}

		PopulationTopologyRewrite(
		    std::move(vertex_descriptor), std::move(old_reference),
		    population_shape.slice(slice_indices), std::move(connections_on_executor),
		    m_topology)();
	}
}

} // namespace grenade::common
