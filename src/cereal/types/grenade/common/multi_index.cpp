#include "grenade/common/multi_index.h"

#include "grenade/cerealization.h"
#include <cereal/types/vector.hpp>

namespace grenade::common {

template <typename Archive>
void MultiIndex::serialize(Archive& ar, std::uint32_t const)
{
	ar(CEREAL_NVP(value));
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::MultiIndex)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::MultiIndex)
CEREAL_CLASS_VERSION(grenade::common::MultiIndex, 0)
