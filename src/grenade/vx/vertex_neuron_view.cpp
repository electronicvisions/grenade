#include "grenade/vx/vertex/neuron_view.h"

#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/synapse_array_view.h"

#include <algorithm>
#include <stdexcept>

namespace grenade::vx::vertex {

NeuronView::NeuronView(Columns const& columns, Row const& row) : m_columns(), m_row(row)
{
	std::set<Columns::value_type> unique(columns.begin(), columns.end());
	if (unique.size() != columns.size()) {
		throw std::runtime_error("Neuron locations provided to NeuronView are not unique.");
	}
	m_columns = columns;
}

NeuronView::Columns const& NeuronView::get_columns() const
{
	return m_columns;
}

NeuronView::Row const& NeuronView::get_row() const
{
	return m_row;
}

std::array<Port, 1> NeuronView::inputs() const
{
	return {Port(m_columns.size(), ConnectionType::SynapticInput)};
}

Port NeuronView::output() const
{
	return Port(m_columns.size(), ConnectionType::MembraneVoltage);
}

std::ostream& operator<<(std::ostream& os, NeuronView const& config)
{
	os << "NeuronView(row: " << config.m_row << ", num_columns: " << config.m_columns.size() << ")";
	return os;
}

bool NeuronView::supports_input_from(
    SynapseArrayView const& input, std::optional<PortRestriction> const& restriction) const
{
	if (input.get_synram() != m_row.toSynramOnDLS()) {
		return false;
	}
	auto const input_columns = input.get_columns();
	if (!restriction) {
		if (input_columns.size() != m_columns.size()) {
			return false;
		}
		return std::equal(
		    input_columns.begin(), input_columns.end(), m_columns.begin(),
		    [](auto const& s, auto const& n) { return s == n.toSynapseOnSynapseRow(); });
	} else {
		if (!restriction->is_restriction_of(input.output())) {
			throw std::runtime_error(
			    "Given restriction is not a restriction of input vertex output port.");
		}
		if (restriction->size() != m_columns.size()) {
			return false;
		}
		return std::equal(
		    input_columns.begin() + restriction->min(),
		    input_columns.begin() + restriction->max() + 1, m_columns.begin(),
		    [](auto const& s, auto const& n) { return s == n.toSynapseOnSynapseRow(); });
	}
}

} // namespace grenade::vx::vertex
