#include "grenade/vx/network/abstract/recorder.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"

namespace grenade::vx::network::abstract {

Recorder::Recorder(
    grenade::common::MultiIndexSequence const& shape,
    grenade::common::TimeDomainOnTopology const& time_domain,
    std::optional<grenade::common::ExecutionInstanceOnExecutor> const&
        execution_instance_on_executor) :
    grenade::common::Recorder(shape, time_domain, execution_instance_on_executor)
{
}

bool Recorder::valid(grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const
{
	return dynamic_cast<ClockCycleTimeDomainRuntimes const*>(&time_domain_runtimes) != nullptr;
}

} // namespace grenade::vx::network::abstract
