#include "grenade/common/multi_index_sequence.h"

#include "cereal/types/dapr/property_holder.h"
#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::common {

template <typename Archive>
void MultiIndexSequence::DimensionUnit::serialize(Archive&, std::uint32_t const)
{
}

template <typename Archive>
void MultiIndexSequence::serialize(Archive&, std::uint32_t const)
{
}

} // namespace grenade::common

CEREAL_REGISTER_DYNAMIC_INIT(grenade_common_multi_index_sequence)

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::MultiIndexSequence::DimensionUnit)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::MultiIndexSequence::DimensionUnit)
CEREAL_REGISTER_TYPE(grenade::common::MultiIndexSequence::DimensionUnit)
CEREAL_CLASS_VERSION(grenade::common::MultiIndexSequence::DimensionUnit, 0)

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::MultiIndexSequence)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::MultiIndexSequence)
CEREAL_REGISTER_TYPE(grenade::common::MultiIndexSequence)
CEREAL_CLASS_VERSION(grenade::common::MultiIndexSequence, 0)
