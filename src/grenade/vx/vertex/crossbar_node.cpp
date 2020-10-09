#include "grenade/vx/vertex/crossbar_node.h"

#include "grenade/cerealization.h"
#include "halco/common/cerealization_geometry.h"
#include <ostream>

namespace grenade::vx::vertex {

CrossbarNode::CrossbarNode(Coordinate const& coordinate, Config const& config) :
    m_coordinate(coordinate), m_config(config)
{}

CrossbarNode::Coordinate const& CrossbarNode::get_coordinate() const
{
	return m_coordinate;
}

CrossbarNode::Config const& CrossbarNode::get_config() const
{
	return m_config;
}

std::ostream& operator<<(std::ostream& os, CrossbarNode const& config)
{
	os << "CrossbarNode(" << std::endl
	   << "coordinate: " << config.m_coordinate << std::endl
	   << "config: " << config.m_config << ")";
	return os;
}

bool CrossbarNode::operator==(CrossbarNode const& other) const
{
	return (m_coordinate == other.m_coordinate) && (m_config == other.m_config);
}

bool CrossbarNode::operator!=(CrossbarNode const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void CrossbarNode::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_coordinate);
	ar(m_config);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::CrossbarNode)
CEREAL_CLASS_VERSION(grenade::vx::vertex::CrossbarNode, 0)
