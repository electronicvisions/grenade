#include "grenade/vx/signal_flow/vertex/pad_readout.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/optional.hpp>
#include <cereal/types/polymorphic.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void PadReadoutView::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<EntityOnChip>(this));
	ar(source);
	ar(coordinate);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::PadReadoutView)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::PadReadoutView, 0)
