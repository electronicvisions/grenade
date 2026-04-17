#include "grenade/vx/signal_flow/vertex/spikeio_source_population.h"

#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void SpikeIOSourcePopulation::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<EntityOnChip>(this));
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::SpikeIOSourcePopulation)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::SpikeIOSourcePopulation, 0)