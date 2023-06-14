#include "grenade/vx/signal_flow/port_restriction.h"

#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow {

template <typename Archive>
void PortRestriction::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_min);
	ar(m_max);
}

} // namespace grenade::vx::signal_flow

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::PortRestriction)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::PortRestriction, 0)
