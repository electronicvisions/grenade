#include "grenade/vx/common/execution_instance_id.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"

namespace grenade::vx::common {

template <typename Archive>
void ExecutionInstanceID::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_execution_index);
	ar(m_dls_global);
}

} // namespace grenade::vx::common

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::common::ExecutionInstanceID)
CEREAL_CLASS_VERSION(grenade::vx::common::ExecutionInstanceID, 0)
