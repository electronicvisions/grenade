#include "grenade/vx/signal_flow/execution_instance.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow {

template <typename Archive>
void ExecutionInstance::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_execution_index);
	ar(m_dls_global);
}

} // namespace grenade::vx::signal_flow

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::ExecutionInstance)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::ExecutionInstance, 0)
