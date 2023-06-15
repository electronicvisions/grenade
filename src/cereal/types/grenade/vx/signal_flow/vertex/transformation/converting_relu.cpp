#include "grenade/vx/signal_flow/vertex/transformation/converting_relu.h"

#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>

namespace grenade::vx::signal_flow::vertex::transformation {

template <typename Archive>
void ConvertingReLU::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
	ar(m_shift);
}

} // namespace grenade::vx::signal_flow::vertex::transformation

CEREAL_REGISTER_TYPE(grenade::vx::signal_flow::vertex::transformation::ConvertingReLU)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::vx::signal_flow::vertex::Transformation::Function,
    grenade::vx::signal_flow::vertex::transformation::ConvertingReLU)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(
    grenade::vx::signal_flow::vertex::transformation::ConvertingReLU)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::transformation::ConvertingReLU, 0)
