#include "grenade/vx/signal_flow/vertex/spikeio_source_population.h"

#include "cereal/types/halco/common/geometry.h"
#include "cereal/types/map.hpp"
#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void SpikeIOSourcePopulation::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<EntityOnChip>(this));
	ar(CEREAL_NVP(m_config));
	ar(CEREAL_NVP(m_output));
	ar(CEREAL_NVP(m_input));
}
} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::SpikeIOSourcePopulation)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::SpikeIOSourcePopulation, 0)