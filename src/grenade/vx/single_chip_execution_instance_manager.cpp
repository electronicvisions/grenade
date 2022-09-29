#include "grenade/vx/single_chip_execution_instance_manager.h"

namespace grenade::vx {

signal_flow::ExecutionInstance SingleChipExecutionInstanceManager::next()
{
	using namespace halco::hicann_dls::vx::v3;
	if (m_current_hemisphere) {
		m_current_index = signal_flow::ExecutionIndex(m_current_index + 1);
	}
	m_current_hemisphere = HemisphereOnDLS((m_current_hemisphere + 1) % HemisphereOnDLS::size);
	return signal_flow::ExecutionInstance(m_current_index, DLSGlobal());
}

halco::hicann_dls::vx::v3::HemisphereOnDLS
SingleChipExecutionInstanceManager::get_current_hemisphere() const
{
	return m_current_hemisphere;
}

signal_flow::ExecutionInstance SingleChipExecutionInstanceManager::next_index()
{
	if (m_current_hemisphere == 0) {
		next();
	}
	return next();
}

} // namespace grenade::vx
