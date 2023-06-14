#include "grenade/vx/signal_flow/transformation/concatenation.h"

#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::vx::signal_flow::transformation {

template <typename Archive>
void Concatenation::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_type);
	ar(m_sizes);
}

} // namespace grenade::vx::signal_flow::transformation

CEREAL_REGISTER_TYPE(grenade::vx::signal_flow::transformation::Concatenation)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::vx::signal_flow::vertex::Transformation::Function,
    grenade::vx::signal_flow::transformation::Concatenation)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::transformation::Concatenation)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::transformation::Concatenation, 0)
