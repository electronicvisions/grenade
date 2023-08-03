#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"

#include "grenade/vx/signal_flow/port_restriction.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

EntityOnChip::EntityOnChip(ChipCoordinate const& coordinate_chip) :
    m_coordinate_chip(coordinate_chip)
{}

EntityOnChip::ChipCoordinate EntityOnChip::get_coordinate_chip() const
{
	return m_coordinate_chip;
}

bool EntityOnChip::operator==(EntityOnChip const& other) const
{
	return m_coordinate_chip == other.m_coordinate_chip;
}

bool EntityOnChip::operator!=(EntityOnChip const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, EntityOnChip const& entity)
{
	return os << "EntityOnChip(" << entity.m_coordinate_chip << ")";
}

} // namespace grenade::vx::signal_flow::vertex
