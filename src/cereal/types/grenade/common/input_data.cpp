#include "grenade/common/input_data.h"

#include "cereal/types/dapr/map.h"
#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/utility.hpp>


namespace grenade::common {

template <typename Archive>
void InputData::serialize(Archive& ar, std::uint32_t const)
{
	ar(ports);
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::InputData)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::InputData)
CEREAL_CLASS_VERSION(grenade::common::InputData, 0)
