#include "grenade/vx/vertex/cadc_membrane_readout_view.h"

#include "grenade/cerealization.h"
#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/neuron_view.h"

#include "halco/common/cerealization_geometry.h"
#include <numeric>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <cereal/types/vector.hpp>

namespace grenade::vx::vertex {

void CADCMembraneReadoutView::check(Columns const& columns)
{
	std::set<Columns::value_type::value_type> unique;
	size_t size = 0;
	for (auto const& column_collection : columns) {
		unique.insert(column_collection.begin(), column_collection.end());
		size += column_collection.size();
	}
	if (unique.size() != size) {
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

std::vector<Port> CADCMembraneReadoutView::inputs() const
{
	std::vector<Port> ret;
	for (auto const& column_collection : m_columns) {
		ret.push_back(Port(column_collection.size(), ConnectionType::MembraneVoltage));
	}
	return ret;
}

Port CADCMembraneReadoutView::output() const
{
	size_t const size = std::accumulate(
	    m_columns.begin(), m_columns.end(), static_cast<size_t>(0),
	    [](auto const& s, auto const& column_collection) { return s + column_collection.size(); });
	return Port(size, ConnectionType::Int8);
}

std::ostream& operator<<(std::ostream& os, CADCMembraneReadoutView const& config)
{
	size_t const size = std::accumulate(
	    config.m_columns.begin(), config.m_columns.end(), static_cast<size_t>(0),
	    [](auto const& s, auto const& column_collection) { return s + column_collection.size(); });
	os << "CADCMembraneReadoutView(size: " << size << ")";
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
		// check if any column collection matches the input
		for (auto const& column_collection : m_columns) {
			if (input_columns.size() != column_collection.size()) {
				continue;
			}
			if (std::equal(
			        input_columns.begin(), input_columns.end(), column_collection.begin(),
			        [](auto const& n, auto const& s) { return s == n.toSynapseOnSynapseRow(); })) {
				return true;
			}
		}
		return false;
	} else {
		if (!restriction->is_restriction_of(input.output())) {
			throw std::runtime_error(
			    "Given restriction is not a restriction of input vertex output port.");
		}
		// check if any column collection matches the input
		for (auto const& column_collection : m_columns) {
			if (column_collection.size() != restriction->size()) {
				continue;
			}
			assert(restriction->max() < input_columns.size());
			if (std::equal(
			        input_columns.begin() + restriction->min(),
			        input_columns.begin() + restriction->max() + 1, column_collection.begin(),
			        [](auto const& n, auto const& s) { return s == n.toSynapseOnSynapseRow(); })) {
				return true;
			}
		}
		return false;
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
CEREAL_CLASS_VERSION(grenade::vx::vertex::CADCMembraneReadoutView, 1)
