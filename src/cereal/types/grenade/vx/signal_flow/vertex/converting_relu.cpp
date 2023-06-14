#include "grenade/vx/signal_flow/vertex/converting_relu.h"

#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void ConvertingReLU::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
	ar(m_shift);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::ConvertingReLU)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::ConvertingReLU, 0)
