#include "grenade/vx/signal_flow/vertex/pad_readout.h"

#include "grenade/vx/signal_flow/port_restriction.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

bool PadReadoutView::Source::operator==(Source const& other) const
{
	return coord == other.coord && type == other.type && enable_buffered == other.enable_buffered;
}

bool PadReadoutView::Source::operator!=(Source const& other) const
{
	return !(*this == other);
}


PadReadoutView::PadReadoutView(
    Source const& source, Coordinate const& coordinate, ChipCoordinate const& chip_coordinate) :
    EntityOnChip(chip_coordinate), m_source(source), m_coordinate(coordinate)
{}

PadReadoutView::Source const& PadReadoutView::get_source() const
{
	return m_source;
}

PadReadoutView::Coordinate const& PadReadoutView::get_coordinate() const
{
	return m_coordinate;
}

std::array<Port, 1> PadReadoutView::inputs() const
{
	return {Port(1, ConnectionType::MembraneVoltage)};
}

Port PadReadoutView::output() const
{
	return Port(1, ConnectionType::ExternalAnalogSignal);
}

std::ostream& operator<<(std::ostream& os, PadReadoutView const& config)
{
	os << "PadReadoutView(source: " << config.m_source.coord << " coord: " << config.m_coordinate
	   << ")";
	return os;
}

bool PadReadoutView::supports_input_from(
    NeuronView const& input, std::optional<PortRestriction> const& restriction) const
{
	if (!static_cast<EntityOnChip const&>(*this).supports_input_from(input, restriction)) {
		return false;
	}
	auto const input_columns = input.get_columns();
	if (!restriction) {
		if (input_columns.size() > 1ul) {
			return false;
		}
		for (auto const& input_column : input_columns) {
			if (m_source.coord != Source::Coord(input_column, input.get_row())) {
				return false;
			}
		}
		return true;
	} else {
		if (!restriction->is_restriction_of(input.output())) {
			throw std::runtime_error(
			    "Given restriction is not a restriction of input vertex output port.");
		}
		if (restriction->size() > 1ul) {
			return false;
		}
		if (m_source.coord !=
		    Source::Coord(input_columns.at(restriction->min()), input.get_row())) {
			return false;
		}
		return true;
	}
}

bool PadReadoutView::operator==(PadReadoutView const& other) const
{
	return (m_source == other.m_source) && (m_coordinate == other.m_coordinate);
}

bool PadReadoutView::operator!=(PadReadoutView const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow::vertex
