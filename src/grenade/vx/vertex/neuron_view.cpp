#include "grenade/vx/vertex/neuron_view.h"

#include "grenade/cerealization.h"
#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/synapse_array_view.h"
#include "halco/common/cerealization_geometry.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <cereal/types/vector.hpp>

namespace grenade::vx::vertex {

void NeuronView::check(Columns const& columns, Configs const& configs)
{
	std::set<Columns::value_type> unique(columns.begin(), columns.end());
	if (unique.size() != columns.size()) {
		throw std::runtime_error("Neuron locations provided to NeuronView are not unique.");
	}
	if (configs.size() != columns.size()) {
		throw std::runtime_error("Neuron config size provided to NeuronView does not "
		                         "match the number of neurons.");
	}
}

NeuronView::Columns const& NeuronView::get_columns() const
{
	return m_columns;
}

NeuronView::Configs const& NeuronView::get_configs() const
{
	return m_configs;
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

bool NeuronView::operator==(NeuronView const& other) const
{
	return (m_columns == other.m_columns) && (m_configs == other.m_configs) &&
	       (m_row == other.m_row);
}

bool NeuronView::operator!=(NeuronView const& other) const
{
	return !(*this == other);
}

bool NeuronView::Config::operator==(NeuronView::Config const& other) const
{
	return (label == other.label) && (enable_reset == other.enable_reset);
}

bool NeuronView::Config::operator!=(NeuronView::Config const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void NeuronView::Config::serialize(Archive& ar, std::uint32_t const)
{
	ar(label);
	ar(enable_reset);
}

template <typename Archive>
void NeuronView::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_columns);
	ar(m_row);
	ar(m_configs);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::NeuronView)
CEREAL_CLASS_VERSION(grenade::vx::vertex::NeuronView, 1)
