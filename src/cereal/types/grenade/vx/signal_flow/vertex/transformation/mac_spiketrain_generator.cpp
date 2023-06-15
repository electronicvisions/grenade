#include "grenade/vx/signal_flow/vertex/transformation/mac_spiketrain_generator.h"

#include "cereal/types/halco/common/geometry.h"
#include "cereal/types/halco/common/typed_array.h"
#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>

namespace grenade::vx::signal_flow::vertex::transformation {

template <typename Archive>
void MACSpikeTrainGenerator::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_hemisphere_sizes);
	ar(m_num_sends);
	ar(m_wait_between_events);
}

} // namespace grenade::vx::signal_flow::vertex::transformation

CEREAL_REGISTER_TYPE(grenade::vx::signal_flow::vertex::transformation::MACSpikeTrainGenerator)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::vx::signal_flow::vertex::Transformation::Function,
    grenade::vx::signal_flow::vertex::transformation::MACSpikeTrainGenerator)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(
    grenade::vx::signal_flow::vertex::transformation::MACSpikeTrainGenerator)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::transformation::MACSpikeTrainGenerator, 0)
