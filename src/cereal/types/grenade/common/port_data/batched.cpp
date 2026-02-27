#include "grenade/common/port_data/batched.h"

#include "grenade/cerealization.h"
#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>

namespace grenade::common {

template <typename Archive>
void BatchedPortData::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<PortData>(this));
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::BatchedPortData)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::BatchedPortData)
CEREAL_CLASS_VERSION(grenade::common::BatchedPortData, 0)
CEREAL_REGISTER_TYPE(grenade::common::BatchedPortData)
CEREAL_REGISTER_POLYMORPHIC_RELATION(grenade::common::PortData, grenade::common::BatchedPortData)
