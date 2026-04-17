#include "grenade/vx/signal_flow/vertex/background_spike_source.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void BackgroundSpikeSource::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<EntityOnChip>(this));
	ar(CEREAL_NVP(coordinate));
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::BackgroundSpikeSource)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::BackgroundSpikeSource, 0)
