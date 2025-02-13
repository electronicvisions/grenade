#include "grenade/common/multi_index_sequence/cartesian_product.h"

#include "cereal/types/dapr/property_holder.h"
#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>

namespace grenade::common {

template <typename Archive>
void CartesianProductMultiIndexSequence::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<MultiIndexSequence>(this));
	ar(CEREAL_NVP(m_first));
	ar(CEREAL_NVP(m_second));
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::CartesianProductMultiIndexSequence)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::CartesianProductMultiIndexSequence)
CEREAL_REGISTER_TYPE(grenade::common::CartesianProductMultiIndexSequence)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::MultiIndexSequence, grenade::common::CartesianProductMultiIndexSequence)
CEREAL_CLASS_VERSION(grenade::common::CartesianProductMultiIndexSequence, 0)
