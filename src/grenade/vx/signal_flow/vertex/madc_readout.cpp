#include "grenade/vx/signal_flow/vertex/madc_readout.h"

#include "grenade/vx/signal_flow/port_restriction.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

bool MADCReadoutView::Source::operator==(Source const& other) const
{
	return coord == other.coord && type == other.type;
}

bool MADCReadoutView::Source::operator!=(Source const& other) const
{
	return !(*this == other);
}

bool MADCReadoutView::SourceSelection::operator==(SourceSelection const& other) const
{
	return initial == other.initial && period == other.period;
}

bool MADCReadoutView::SourceSelection::operator!=(SourceSelection const& other) const
{
	return !(*this == other);
}

MADCReadoutView::MADCReadoutView(
    Source const& first_source,
    std::optional<Source> const& second_source,
    SourceSelection const& source_selection,
    ChipCoordinate const& chip_coordinate) :
    EntityOnChip(chip_coordinate)
{
	if (!second_source &&
	    ((source_selection.period.value() != 0) || (source_selection.initial.value() != 0))) {
		throw std::runtime_error("MADCReadoutView source swapping only possible for two sources.");
	}
	if (second_source) {
		if ((first_source.coord.toNeuronColumnOnDLS().value() % 2) ==
		    second_source->coord.toNeuronColumnOnDLS().value() % 2) {
			throw std::runtime_error(
			    "MADCReadoutView sources can't both be located in even or odd neuron columns.");
		}
	}
	if (source_selection.period > SourceSelection::Period(1)) {
		throw std::runtime_error(
		    "MADCReadoutView source switch period > 1 not supported due to Issue #3998.");
	}
	m_first_source = first_source;
	m_second_source = second_source;
	m_source_selection = source_selection;
}

MADCReadoutView::Source const& MADCReadoutView::get_first_source() const
{
	return m_first_source;
}

std::optional<MADCReadoutView::Source> const& MADCReadoutView::get_second_source() const
{
	return m_second_source;
}

MADCReadoutView::SourceSelection const& MADCReadoutView::get_source_selection() const
{
	return m_source_selection;
}

std::vector<Port> MADCReadoutView::inputs() const
{
	std::vector<Port> ret;
	ret.push_back(Port(1, ConnectionType::MembraneVoltage));
	if (m_second_source) {
		ret.push_back(Port(1, ConnectionType::MembraneVoltage));
	}
	return ret;
}

Port MADCReadoutView::output() const
{
	return Port(1, ConnectionType::TimedMADCSampleFromChipSequence);
}

std::ostream& operator<<(std::ostream& os, MADCReadoutView const& config)
{
	os << "MADCReadoutView(";
	if (config.m_second_source) {
		os << "MADCReadoutView(first_coord: " << config.m_first_source.coord
		   << ", second_coord: " << config.m_second_source->coord << ")";
	} else {
		os << "MADCReadoutView(coord: " << config.m_first_source.coord << ")";
	}
	return os;
}

bool MADCReadoutView::supports_input_from(
    NeuronView const& input, std::optional<PortRestriction> const& restriction) const
{
	if (!static_cast<EntityOnChip const&>(*this).supports_input_from(input, restriction)) {
		return false;
	}
	std::set<Source::Coord> coords;
	coords.insert(m_first_source.coord);
	if (m_second_source) {
		coords.insert(m_second_source->coord);
	}
	auto const input_columns = input.get_columns();
	if (!restriction) {
		if (input_columns.size() > 1ul + static_cast<bool>(m_second_source)) {
			return false;
		}
		for (auto const& input_column : input_columns) {
			if (!coords.contains(Source::Coord(input_column, input.get_row()))) {
				return false;
			}
		}
		return true;
	} else {
		if (!restriction->is_restriction_of(input.output())) {
			throw std::runtime_error(
			    "Given restriction is not a restriction of input vertex output port.");
		}
		if (restriction->size() > 1ul + static_cast<bool>(m_second_source)) {
			return false;
		}
		if (!coords.contains(
		        Source::Coord(input_columns.at(restriction->min()), input.get_row()))) {
			return false;
		}
		if (!coords.contains(
		        Source::Coord(input_columns.at(restriction->max()), input.get_row()))) {
			return false;
		}
		return true;
	}
}

bool MADCReadoutView::operator==(MADCReadoutView const& other) const
{
	return (m_first_source == other.m_first_source) && (m_second_source == other.m_second_source) &&
	       (m_source_selection == other.m_source_selection);
}

bool MADCReadoutView::operator!=(MADCReadoutView const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow::vertex
