#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/recorder.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Recorder in topology.
 */
struct SYMBOL_VISIBLE GENPYBIND(
    visible, holder_type("std::shared_ptr<grenade::vx::network::abstract::Recorder>")) Recorder
    : public grenade::common::Recorder
{
	/**
	 * Construct recorder with specified number of channels and time-domain.
	 * @param shape Shape of source channels to record
	 * @param time_domain Time domain onto which to place recorder
	 * @param execution_instance_on_executor Execution instance on executor onto which to partition
	 * recorder
	 */
	Recorder(
	    grenade::common::MultiIndexSequence const& shape,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    std::optional<grenade::common::ExecutionInstanceOnExecutor> const&
	        execution_instance_on_executor = std::nullopt);

	/**
	 * Only ClockCycleTimeDomainRuntimes are valid.
	 */
	virtual bool valid(
	    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
