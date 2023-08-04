#include "grenade/vx/compute/detail/single_chip_execution_instance_manager.h"

namespace grenade::vx::compute::detail {

common::ExecutionInstanceID SingleChipExecutionInstanceManager::next()
{
	using namespace halco::hicann_dls::vx::v3;
	if (m_current_hemisphere) {
		m_current_instance = common::ExecutionInstanceID(m_current_instance + 1);
	}
	m_current_hemisphere = HemisphereOnDLS((m_current_hemisphere + 1) % HemisphereOnDLS::size);
	return m_current_instance;
}

halco::hicann_dls::vx::v3::HemisphereOnDLS
SingleChipExecutionInstanceManager::get_current_hemisphere() const
{
	return m_current_hemisphere;
}

common::ExecutionInstanceID SingleChipExecutionInstanceManager::next_index()
{
	if (m_current_hemisphere == 0) {
		next();
	}
	return next();
}

} // namespace grenade::vx::compute::detail
