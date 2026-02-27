#include "grenade/common/port_data.h"

#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>

namespace grenade::common {

template <typename Archive>
void PortData::serialize(Archive&, std::uint32_t const)
{
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::PortData)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::PortData)
CEREAL_CLASS_VERSION(grenade::common::PortData, 0)
CEREAL_REGISTER_TYPE(grenade::common::PortData)
