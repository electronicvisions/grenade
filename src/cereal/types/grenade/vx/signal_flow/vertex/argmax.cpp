#include "grenade/vx/signal_flow/vertex/argmax.h"

#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void ArgMax::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
	ar(m_type);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::ArgMax)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::ArgMax, 0)
