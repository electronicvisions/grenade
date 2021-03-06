#include "grenade/vx/vertex/cadc_membrane_readout_view.h"

#include "grenade/cerealization.h"
#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/neuron_view.h"

#include "halco/common/cerealization_geometry.h"
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <cereal/types/vector.hpp>

namespace grenade::vx::vertex {

void CADCMembraneReadoutView::check(Columns const& columns)
{
	std::set<Columns::value_type> unique(columns.begin(), columns.end());
	if (unique.size() != columns.size()) {
		throw std::runtime_error("Column locations provided to CADCReadoutView are not unique.");
	}
}

CADCMembraneReadoutView::Columns const& CADCMembraneReadoutView::get_columns() const
{
	return m_columns;
}

CADCMembraneReadoutView::Synram const& CADCMembraneReadoutView::get_synram() const
{
	return m_synram;
}

std::array<Port, 1> CADCMembraneReadoutView::inputs() const
{
	return {Port(m_columns.size(), ConnectionType::MembraneVoltage)};
}

Port CADCMembraneReadoutView::output() const
{
	return Port(m_columns.size(), ConnectionType::Int8);
}

std::ostream& operator<<(std::ostream& os, CADCMembraneReadoutView const& config)
{
	os << "CADCMembraneReadoutView(size: " << config.m_columns.size() << ")";
	return os;
}

bool CADCMembraneReadoutView::supports_input_from(
    NeuronView const& input, std::optional<PortRestriction> const& restriction) const
{
	if (input.get_row().toSynramOnDLS() != m_synram) {
		return false;
	}
	auto const input_columns = input.get_columns();
	if (!restriction) {
		if (input_columns.size() != m_columns.size()) {
			return false;
		}
		return std::equal(
		    input_columns.begin(), input_columns.end(), m_columns.begin(),
		    [](auto const& n, auto const& s) { return s == n.toSynapseOnSynapseRow(); });
	} else {
		if (!restriction->is_restriction_of(input.output())) {
			throw std::runtime_error(
			    "Given restriction is not a restriction of input vertex output port.");
		}
		if (m_columns.size() != restriction->size()) {
			return false;
		}
		return std::equal(
		    input_columns.begin() + restriction->min(),
		    input_columns.begin() + restriction->max() + 1, m_columns.begin(),
		    [](auto const& n, auto const& s) { return s == n.toSynapseOnSynapseRow(); });
	}
}

bool CADCMembraneReadoutView::operator==(CADCMembraneReadoutView const& other) const
{
	return (m_columns == other.m_columns) && (m_synram == other.m_synram);
}

bool CADCMembraneReadoutView::operator!=(CADCMembraneReadoutView const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void CADCMembraneReadoutView::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_columns);
	ar(m_synram);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::CADCMembraneReadoutView)
CEREAL_CLASS_VERSION(grenade::vx::vertex::CADCMembraneReadoutView, 0)
