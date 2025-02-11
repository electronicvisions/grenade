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
	 * @param resource_estimator Estimator for resource requirements of a population
	 * @param system_resources Resources of one system instance (resources per connection on the
	 * executor) onto which to fit each split
	 * @param topology Linked topology
	 */
	PopulationTopologyRewrite(
	    ResourceEstimator const& resource_estimator,
	    SystemResources const& system_resources,
	    std::shared_ptr<LinkedTopology> topology);

	virtual void operator()() const override;

private:
	std::unique_ptr<ResourceEstimator> m_resource_estimator;
	SystemResources m_system_resources;
};

} // namespace grenade::common
