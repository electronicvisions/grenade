#include "grenade/vx/signal_flow/vertex/padi_bus.h"

#include "grenade/vx/signal_flow/port_restriction.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

PADIBus::PADIBus(Coordinate const& coordinate, ChipOnExecutor const& chip_on_executor) :
    EntityOnChip(chip_on_executor), m_coordinate(coordinate)
{}

bool PADIBus::supports_input_from(
    CrossbarNode const& input, std::optional<PortRestriction> const& port_restriction) const
{
	return static_cast<EntityOnChip const&>(*this).supports_input_from(input, port_restriction) &&
	       (m_coordinate.toCrossbarOutputOnDLS() == input.get_coordinate().toCrossbarOutputOnDLS());
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

bool PADIBus::operator==(PADIBus const& other) const
{
	return m_coordinate == other.m_coordinate;
}

bool PADIBus::operator!=(PADIBus const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow::vertex
