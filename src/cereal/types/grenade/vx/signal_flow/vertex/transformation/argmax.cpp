#include "grenade/vx/signal_flow/vertex/transformation/argmax.h"

#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>

namespace grenade::vx::signal_flow::vertex::transformation {

template <typename Archive>
void ArgMax::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
	ar(m_type);
}

} // namespace grenade::vx::signal_flow::vertex::transformation

CEREAL_REGISTER_TYPE(grenade::vx::signal_flow::vertex::transformation::ArgMax)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::vx::signal_flow::vertex::Transformation::Function,
    grenade::vx::signal_flow::vertex::transformation::ArgMax)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::transformation::ArgMax)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::transformation::ArgMax, 0)
