#include "grenade/vx/signal_flow/vertex/crossbar_node.h"

#include "grenade/vx/signal_flow/vertex/background_spike_source.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

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

bool CrossbarNode::supports_input_from(
    BackgroundSpikeSource const& input, std::optional<PortRestriction> const&) const
{
	return m_coordinate.toCrossbarInputOnDLS() == input.get_coordinate().toCrossbarInputOnDLS();
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

} // namespace grenade::vx::signal_flow::vertex
