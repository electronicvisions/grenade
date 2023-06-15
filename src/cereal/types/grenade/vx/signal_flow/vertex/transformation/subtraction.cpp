#include "grenade/vx/signal_flow/vertex/transformation/subtraction.h"

#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>

namespace grenade::vx::signal_flow::vertex::transformation {

template <typename Archive>
void Subtraction::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_num_inputs);
	ar(m_size);
}

} // namespace grenade::vx::signal_flow::vertex::transformation

CEREAL_REGISTER_TYPE(grenade::vx::signal_flow::vertex::transformation::Subtraction)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::vx::signal_flow::vertex::Transformation::Function,
    grenade::vx::signal_flow::vertex::transformation::Subtraction)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::transformation::Subtraction)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::transformation::Subtraction, 0)
