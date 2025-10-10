#include "grenade/common/vertex.h"

#include "cereal/types/dapr/property_holder.h"
#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>

namespace grenade::common {

template <typename Archive>
void Vertex::Port::serialize(Archive& ar, std::uint32_t const)
{
	ar(sum_or_split_support);
	ar(execution_instance_transition_constraint);
	ar(requires_or_generates_data);
	ar(m_channels);
	ar(m_type);
}

template <typename Archive>
void Vertex::serialize(Archive&, std::uint32_t const)
{
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::Vertex::Port)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::Vertex::Port)
CEREAL_CLASS_VERSION(grenade::common::Vertex::Port, 0)

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::Vertex)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::Vertex)
CEREAL_CLASS_VERSION(grenade::common::Vertex, 0)
CEREAL_REGISTER_TYPE(grenade::common::Vertex)
