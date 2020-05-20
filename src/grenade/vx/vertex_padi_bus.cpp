#include "grenade/vx/vertex/padi_bus.h"

#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/crossbar_node.h"

namespace grenade::vx::vertex {

PADIBus::PADIBus(Coordinate const& coordinate) : m_coordinate(coordinate) {}

bool PADIBus::supports_input_from(
    CrossbarNode const& input, std::optional<PortRestriction> const&) const
{
	return m_coordinate.toCrossbarOutputOnDLS() == input.get_coordinate().toCrossbarOutputOnDLS();
}

PADIBus::Coordinate const& PADIBus::get_coordinate() const
{
	return m_coordinate;
}

std::ostream& operator<<(std::ostream& os, PADIBus const& config)
{
	os << "PADIBus(coordinate: " << config.m_coordinate << ")";
	return os;
}

} // namespace grenade::vx::vertex
