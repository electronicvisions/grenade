#include "grenade/common/edge.h"

#include "cereal/types/dapr/property_holder.h"
#include "grenade/cerealization.h"

namespace grenade::common {

template <typename Archive>
void Edge::serialize(Archive& ar, std::uint32_t const)
{
	ar(port_on_source);
	ar(port_on_target);
	ar(m_channels_on_source);
	ar(m_channels_on_target);
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::Edge)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::Edge)
CEREAL_CLASS_VERSION(grenade::common::Edge, 0)
