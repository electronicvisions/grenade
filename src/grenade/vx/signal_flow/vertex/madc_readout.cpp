#include "grenade/vx/signal_flow/vertex/madc_readout.h"

#include "grenade/cerealization.h"
#include "grenade/vx/signal_flow/port_restriction.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "halco/common/cerealization_geometry.h"
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

MADCReadoutView::MADCReadoutView(Coord const& coord, Config const& config) :
    m_coord(coord), m_config(config)
{}

MADCReadoutView::Coord const& MADCReadoutView::get_coord() const
{
	return m_coord;
}

MADCReadoutView::Config const& MADCReadoutView::get_config() const
{
	return m_config;
}

std::array<Port, 1> MADCReadoutView::inputs() const
{
	return {Port(1, ConnectionType::MembraneVoltage)};
}

Port MADCReadoutView::output() const
{
	return Port(1, ConnectionType::TimedMADCSampleFromChipSequence);
}

std::ostream& operator<<(std::ostream& os, MADCReadoutView const& config)
{
	os << "MADCReadoutView(coord: " << config.m_coord << ")";
	return os;
}

bool MADCReadoutView::supports_input_from(
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

bool MADCReadoutView::operator==(MADCReadoutView const& other) const
{
	return (m_coord == other.m_coord) && (m_config == other.m_config);
}

bool MADCReadoutView::operator!=(MADCReadoutView const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void MADCReadoutView::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_coord);
	ar(m_config);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::MADCReadoutView)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::MADCReadoutView, 0)
