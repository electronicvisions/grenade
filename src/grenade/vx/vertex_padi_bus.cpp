#include "grenade/vx/vertex/padi_bus.h"

#include "grenade/cerealization.h"
#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/crossbar_node.h"
#include "halco/common/cerealization_geometry.h"

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

bool PADIBus::operator==(PADIBus const& other) const
{
	return m_coordinate == other.m_coordinate;
}

bool PADIBus::operator!=(PADIBus const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void PADIBus::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_coordinate);
}


} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::PADIBus)
CEREAL_CLASS_VERSION(grenade::vx::vertex::PADIBus, 0)
