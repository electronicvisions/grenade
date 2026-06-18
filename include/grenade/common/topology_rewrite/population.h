#pragma once
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/resource_estimator.h"
#include "grenade/common/topology_rewrite.h"
#include "hate/visibility.h"

namespace grenade::common {

/**
 * Population rewrite, which splits population in intervals and assigns connections on the
 * executor round-robin.
 */
struct SYMBOL_VISIBLE PopulationTopologyRewrite : public TopologyRewrite
{
	typedef std::map<ConnectionOnExecutor, dapr::PropertyHolder<ResourceEstimator::Resource>>
	    SystemResources;

	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param vertex_descriptor Population descriptor for which to perform rewrite
	 * @param old_reference Old reference of the population
	 * @param new_population_shapes Shapes of populations in which to split old population
	 * @param connections_on_executor Connections on which population is split
	 * @param topology Linked topology
	 */
	PopulationTopologyRewrite(
	    VertexOnTopology vertex_descriptor,
	    LinkedTopology::ReferenceGraph::VertexDescriptor old_reference,
	    std::vector<std::unique_ptr<MultiIndexSequence>> new_population_shapes,
	    std::vector<ConnectionOnExecutor> connections_on_executor,
	    std::shared_ptr<LinkedTopology> topology);

	virtual void operator()() const override;

private:
	VertexOnTopology m_vertex_descriptor;
	LinkedTopology::ReferenceGraph::VertexDescriptor m_old_reference;
	std::vector<std::unique_ptr<MultiIndexSequence>> m_new_population_shapes;
	std::vector<ConnectionOnExecutor> m_connections_on_executor;
};

} // namespace grenade::common
