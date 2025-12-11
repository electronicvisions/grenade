#include "grenade/common/execution_instance_on_executor.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"

namespace grenade::common {

template <typename Archive>
void ExecutionInstanceOnExecutor::serialize(Archive& ar, std::uint32_t const)
{
	ar(CEREAL_NVP(execution_instance_id));
	ar(CEREAL_NVP(connection_on_executor));
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::ExecutionInstanceOnExecutor)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::ExecutionInstanceOnExecutor)
CEREAL_CLASS_VERSION(grenade::common::ExecutionInstanceOnExecutor, 0)
