#include "grenade/vx/compute/sequence.h"

#include "grenade/cerealization.h"
#include <cereal/types/list.hpp>
#include <cereal/types/variant.hpp>

namespace grenade::vx::compute {

template <typename Archive>
void Sequence::serialize(Archive& ar, std::uint32_t const)
{
	ar(data);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::Sequence)
CEREAL_CLASS_VERSION(grenade::vx::compute::Sequence, 0)
