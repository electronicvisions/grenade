#include "grenade/common/vertex_port_type.h"

#include "grenade/cerealization.h"
#include <cereal/types/vector.hpp>

namespace grenade::common {

template <typename Archive>
void VertexPortType::serialize(Archive&, std::uint32_t const)
{
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::VertexPortType)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::VertexPortType)
CEREAL_CLASS_VERSION(grenade::common::VertexPortType, 0)
