#include "grenade/common/multi_index_sequence/cuboid.h"

#include "cereal/types/dapr/property_holder.h"
#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::common {

template <typename Archive>
void CuboidMultiIndexSequence::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<MultiIndexSequence>(this));
	ar(CEREAL_NVP(m_shape));
	ar(CEREAL_NVP(m_origin));
	ar(CEREAL_NVP(m_dimension_units));
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::CuboidMultiIndexSequence)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::CuboidMultiIndexSequence)
CEREAL_REGISTER_TYPE(grenade::common::CuboidMultiIndexSequence)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::MultiIndexSequence, grenade::common::CuboidMultiIndexSequence)
CEREAL_CLASS_VERSION(grenade::common::CuboidMultiIndexSequence, 0)
