#include "grenade/vx/vertex/cadc_readout.h"

#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/neuron_view.h"

#include <sstream>
#include <stdexcept>

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

} // namespace grenade::vx::vertex
