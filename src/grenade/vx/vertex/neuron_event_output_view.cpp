#include "grenade/vx/vertex/neuron_event_output_view.h"

#include "grenade/cerealization.h"
#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/neuron_view.h"
#include "halco/common/cerealization_geometry.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <cereal/types/vector.hpp>

namespace grenade::vx::vertex {

NeuronEventOutputView::NeuronEventOutputView(Columns const& columns, Row const& row) :
    m_columns(), m_row(row)
{
	std::set<Columns::value_type> unique(columns.begin(), columns.end());
	if (unique.size() != columns.size()) {
		throw std::runtime_error(
		    "Neuron locations provided to NeuronEventOutputView are not unique.");
	}
	m_columns = columns;
}

NeuronEventOutputView::Columns const& NeuronEventOutputView::get_columns() const
{
	return m_columns;
}

NeuronEventOutputView::Row const& NeuronEventOutputView::get_row() const
{
	return m_row;
}

std::array<Port, 1> NeuronEventOutputView::inputs() const
{
	return {Port(m_columns.size(), ConnectionType::MembraneVoltage)};
}

Port NeuronEventOutputView::output() const
{
	std::set<halco::hicann_dls::vx::v2::NeuronEventOutputOnDLS> outputs;
	for (auto const& column : m_columns) {
		outputs.insert(column.toNeuronEventOutputOnDLS());
	}
	return Port(outputs.size(), ConnectionType::CrossbarInputLabel);
}

std::ostream& operator<<(std::ostream& os, NeuronEventOutputView const& config)
{
	os << "NeuronEventOutputView(row: " << config.m_row
	   << ", num_columns: " << config.m_columns.size() << ")";
	return os;
}

bool NeuronEventOutputView::supports_input_from(
    NeuronView const& input, std::optional<PortRestriction> const& restriction) const
{
	if (input.get_row() != m_row) {
		return false;
	}
	auto const input_columns = input.get_columns();
	if (!restriction) {
		if (input_columns.size() != m_columns.size()) {
			return false;
		}
		return std::equal(input_columns.begin(), input_columns.end(), m_columns.begin());
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
		    input_columns.begin() + restriction->max() + 1, m_columns.begin());
	}
}

bool NeuronEventOutputView::operator==(NeuronEventOutputView const& other) const
{
	return (m_columns == other.m_columns) && (m_row == other.m_row);
}

bool NeuronEventOutputView::operator!=(NeuronEventOutputView const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void NeuronEventOutputView::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_columns);
	ar(m_row);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::NeuronEventOutputView)
CEREAL_CLASS_VERSION(grenade::vx::vertex::NeuronEventOutputView, 0)
