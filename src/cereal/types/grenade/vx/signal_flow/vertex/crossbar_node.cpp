#include "grenade/vx/signal_flow/vertex/crossbar_node.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void CrossbarNode::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_coordinate);
	ar(m_config);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::CrossbarNode)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::CrossbarNode, 0)
