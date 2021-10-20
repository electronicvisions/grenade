#include "grenade/vx/vertex/synapse_array_view_sparse.h"

#include "grenade/cerealization.h"
#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "halco/common/cerealization_geometry.h"
#include "halco/hicann-dls/vx/v2/synapse.h"
#include "halco/hicann-dls/vx/v2/synapse_driver.h"
#include <ostream>
#include <stdexcept>
#include <cereal/types/vector.hpp>

namespace grenade::vx::vertex {

bool SynapseArrayViewSparse::Synapse::operator==(Synapse const& other) const
{
	return (label == other.label) && (weight == other.weight) && (index_row == other.index_row) &&
	       (index_column == other.index_column);
}

bool SynapseArrayViewSparse::Synapse::operator!=(Synapse const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void SynapseArrayViewSparse::Synapse::serialize(Archive& ar, std::uint32_t const)
{
	ar(weight);
	ar(label);
	ar(index_row);
	ar(index_column);
}


void SynapseArrayViewSparse::check(
    Rows const& rows, Columns const& columns, Synapses const& synapses)
{
	if (rows.empty()) {
		throw std::runtime_error("Row size(0) unsupported.");
	}
	if (columns.empty()) {
		throw std::runtime_error("Column size(0) unsupported.");
	}
	std::set<Columns::value_type> unique_columns(columns.begin(), columns.end());
	if (unique_columns.size() != columns.size()) {
		throw std::runtime_error(
		    "Column locations provided to SynapseArrayViewSparse are not unique.");
	}
	std::set<Rows::value_type> unique_rows(rows.begin(), rows.end());
	if (unique_rows.size() != rows.size()) {
		throw std::runtime_error(
		    "Row locations provided to SynapseArrayViewSparse are not unique.");
	}
	std::set<std::pair<size_t, size_t>> unique_synapses;
	for (auto const& synapse : synapses) {
		if (synapse.index_row >= rows.size()) {
			throw std::runtime_error("Synapse row index out of range.");
		}
		if (synapse.index_column >= columns.size()) {
			throw std::runtime_error("Synapse column index out of range.");
		}
		unique_synapses.insert({synapse.index_row, synapse.index_column});
	}
	if (unique_synapses.size() != synapses.size()) {
		throw std::runtime_error(
		    "Synapse locations provided to SynapseArrayViewSparse are not unique.");
	}
}

boost::iterator_range<SynapseArrayViewSparse::Rows::const_iterator>
SynapseArrayViewSparse::get_rows() const
{
	return {m_rows.begin(), m_rows.end()};
}

boost::iterator_range<SynapseArrayViewSparse::Columns::const_iterator>
SynapseArrayViewSparse::get_columns() const
{
	return {m_columns.begin(), m_columns.end()};
}

boost::iterator_range<SynapseArrayViewSparse::Synapses::const_iterator>
SynapseArrayViewSparse::get_synapses() const
{
	return {m_synapses.begin(), m_synapses.end()};
}

SynapseArrayViewSparse::Synram const& SynapseArrayViewSparse::get_synram() const
{
	return m_synram;
}

ppu::SynapseArrayViewHandle SynapseArrayViewSparse::toSynapseArrayViewHandle() const
{
	if (m_synapses.size() != m_rows.size() * m_columns.size()) {
		throw std::runtime_error(
		    "Conversion to SynapseArrayViewHandle only supported for dense connectivity.");
	}
	ppu::SynapseArrayViewHandle result;
	for (auto const& column : m_columns) {
		result.columns.set(column.value());
	}
	for (auto const& row : m_rows) {
		result.rows.set(row.value());
	}
	return result;
}

std::vector<Port> SynapseArrayViewSparse::inputs() const
{
	Port const port(1, ConnectionType::SynapseInputLabel);
	std::set<halco::hicann_dls::vx::v2::SynapseDriverOnSynapseDriverBlock> drivers;
	std::for_each(m_rows.begin(), m_rows.end(), [&](auto const& r) {
		drivers.insert(r.toSynapseDriverOnSynapseDriverBlock());
	});
	std::vector<Port> ret;
	ret.insert(ret.begin(), drivers.size(), port);
	return ret;
}

Port SynapseArrayViewSparse::output() const
{
	return Port(m_columns.size(), ConnectionType::SynapticInput);
}

std::ostream& operator<<(std::ostream& os, SynapseArrayViewSparse const& config)
{
	os << "SynapseArrayViewSparse(size: [rows: " << config.m_rows.size()
	   << ", columns: " << config.m_columns.size() << "]" << std::endl;
	for (auto const& synapse : config.m_synapses) {
		os << "(" << config.m_rows.at(synapse.index_row) << ", "
		   << config.m_columns.at(synapse.index_column) << "): " << synapse.weight << ", "
		   << synapse.label << std::endl;
	}
	os << ")";
	return os;
}

bool SynapseArrayViewSparse::operator==(SynapseArrayViewSparse const& other) const
{
	return (m_synram == other.m_synram) && (m_rows == other.m_rows) &&
	       (m_columns == other.m_columns) && (m_synapses == other.m_synapses);
}

bool SynapseArrayViewSparse::operator!=(SynapseArrayViewSparse const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void SynapseArrayViewSparse::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_synapses);
	ar(m_columns);
	ar(m_rows);
	ar(m_synram);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::SynapseArrayViewSparse)
CEREAL_CLASS_VERSION(grenade::vx::vertex::SynapseArrayViewSparse, 0)
