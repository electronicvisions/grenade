#include "grenade/vx/common/entity_on_chip.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"

namespace grenade::vx::common {

template <typename Archive>
void EntityOnChip::serialize(Archive& ar, std::uint32_t const)
{
	ar(CEREAL_NVP(chip_coordinate));
}

} // namespace grenade::vx::common

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::common::EntityOnChip)
CEREAL_CLASS_VERSION(grenade::vx::common::EntityOnChip, 0)
