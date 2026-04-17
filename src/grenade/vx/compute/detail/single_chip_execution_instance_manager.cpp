#include "grenade/vx/compute/detail/single_chip_execution_instance_manager.h"
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"

namespace grenade::vx::compute::detail {

grenade::common::ExecutionInstanceOnExecutor SingleChipExecutionInstanceManager::next()
{
	using namespace halco::hicann_dls::vx::v3;
	if (m_current_hemisphere) {
		m_current_instance = grenade::common::ExecutionInstanceOnExecutor(
		    grenade::common::ExecutionInstanceID(m_current_instance.execution_instance_id + 1),
		    grenade::common::ConnectionOnExecutor());
	}
	m_current_hemisphere = HemisphereOnDLS((m_current_hemisphere + 1) % HemisphereOnDLS::size);
	return m_current_instance;
}

halco::hicann_dls::vx::v3::HemisphereOnDLS
SingleChipExecutionInstanceManager::get_current_hemisphere() const
{
	return m_current_hemisphere;
}

grenade::common::ExecutionInstanceOnExecutor SingleChipExecutionInstanceManager::next_index()
{
	if (m_current_hemisphere == 0) {
		next();
	}
	return next();
}

} // namespace grenade::vx::compute::detail
