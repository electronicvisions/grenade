#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"

#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void CrossbarL2Output::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<EntityOnChip>(this));
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::CrossbarL2Output)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::CrossbarL2Output, 0)
