#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void EntityOnChip::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<common::EntityOnChip>(this));
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::EntityOnChip)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::EntityOnChip, 0)
