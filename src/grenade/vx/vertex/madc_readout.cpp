#include "grenade/vx/vertex/madc_readout.h"

#include "grenade/cerealization.h"
#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/neuron_view.h"
#include "halco/common/cerealization_geometry.h"
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace grenade::vx::vertex {

MADCMembraneReadoutView::MADCMembraneReadoutView(Coord const& coord) : m_coord(coord) {}

MADCMembraneReadoutView::Coord const& MADCMembraneReadoutView::get_coord() const
{
	return m_coord;
}

std::array<Port, 1> MADCMembraneReadoutView::inputs() const
{
	return {Port(1, ConnectionType::MembraneVoltage)};
}

Port MADCMembraneReadoutView::output() const
{
	return Port(1, ConnectionType::TimedMADCSampleFromChipSequence);
}

std::ostream& operator<<(std::ostream& os, MADCMembraneReadoutView const& config)
{
	os << "MADCMembraneReadoutView(coord: " << config.m_coord << ")";
	return os;
}

bool MADCMembraneReadoutView::supports_input_from(
    NeuronView const& input, std::optional<PortRestriction> const& restriction) const
{
	auto const input_columns = input.get_columns();
	if (!restriction) {
		if (input_columns.size() != 1) {
			return false;
		}
		return Coord(input_columns.at(0), input.get_row()) == m_coord;
	} else {
		if (!restriction->is_restriction_of(input.output())) {
			throw std::runtime_error(
			    "Given restriction is not a restriction of input vertex output port.");
		}
		if (restriction->size() != 1) {
			return false;
		}
		return Coord(input_columns.at(restriction->min()), input.get_row()) == m_coord;
	}
}

bool MADCMembraneReadoutView::operator==(MADCMembraneReadoutView const& other) const
{
	return m_coord == other.m_coord;
}

bool MADCMembraneReadoutView::operator!=(MADCMembraneReadoutView const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void MADCMembraneReadoutView::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_coord);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::MADCMembraneReadoutView)
CEREAL_CLASS_VERSION(grenade::vx::vertex::MADCMembraneReadoutView, 0)
