#include "grenade/vx/signal_flow/vertex/subtraction.h"

#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void Subtraction::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::Subtraction)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::Subtraction, 0)
