#pragma once
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/resource_estimator.h"
#include "grenade/common/topology_rewrite.h"
#include "grenade/common/topology_rewrite/population.h"
#include "hate/visibility.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Execution instance rewrite, which assigns execution instance IDs to vertices.
 * First, it sequentially assigns a unique ID to strongly connected components.
 * Then, differing execution instances across vertices which are connected without allowing an
 * execution instance transition are merged.
 * Finally, execution instances of the same time domain are merged greedy-linear until their
 * resource requirements would surpass the resources per execution instance.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) ExecutionInstanceTopologyRewrite : public TopologyRewrite
{
	typedef PopulationTopologyRewrite::SystemResources SystemResources;

	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param resource_estimator Estimator for resource requirements of a population
	 * @param system_resources Resources of one system instance (resources per connection on the
	 * executor) onto which to partition all execution instances
	 * @param topology Linked topology
	 */
	ExecutionInstanceTopologyRewrite(
	    ResourceEstimator const& resource_estimator,
	    SystemResources const& system_resources,
	    std::shared_ptr<LinkedTopology> topology);

	virtual void operator()() const override;

private:
	std::unique_ptr<ResourceEstimator> m_resource_estimator;
	SystemResources m_system_resources;
};

} // namespace common
} // namespace grenade
