#include "grenade/vx/vertex/synapse_array_view.h"

#include <stdexcept>
#include "halco/hicann-dls/vx/v1/synapse.h"
#include "halco/hicann-dls/vx/v1/synapse_driver.h"

namespace grenade::vx::vertex {

SynapseArrayView::SynapseArrayView(
    Synram const& synram,
    Rows const& rows,
    Columns const& columns,
    Weights const& weights,
    Labels const& labels) :
    m_synram(synram), m_rows(), m_columns(), m_weights(), m_labels()
{
	if (rows.empty()) {
		throw std::runtime_error("Row size(0) unsupported.");
	}
	if (columns.empty()) {
		throw std::runtime_error("Column size(0) unsupported.");
	}
	std::set<Columns::value_type> unique_columns(columns.begin(), columns.end());
	if (unique_columns.size() != columns.size()) {
		throw std::runtime_error("Column locations provided to SynapseArrayView are not unique.");
	}
	std::set<Rows::value_type> unique_rows(rows.begin(), rows.end());
	if (unique_rows.size() != rows.size()) {
		throw std::runtime_error("Row locations provided to SynapseArrayView are not unique.");
	}
	if (rows.size() != weights.size()) {
		throw std::runtime_error("Weight row size does not match rows size.");
	}
	if (rows.size() != labels.size()) {
		throw std::runtime_error("Label row size does not match rows size.");
	}
	for (auto const& row : weights) {
		if (row.size() != columns.size()) {
			throw std::runtime_error("Weight row size does not match columns size.");
		}
	}
	for (auto const& row : labels) {
		if (row.size() != columns.size()) {
			throw std::runtime_error("Weight row size does not match columns size.");
		}
	}
	m_rows = rows;
	m_columns = columns;
	m_weights = weights;
	m_labels = labels;
}

boost::iterator_range<SynapseArrayView::Rows::const_iterator> SynapseArrayView::get_rows() const
{
	return {m_rows.begin(), m_rows.end()};
}

boost::iterator_range<SynapseArrayView::Columns::const_iterator> SynapseArrayView::get_columns()
    const
{
	return {m_columns.begin(), m_columns.end()};
}

boost::iterator_range<SynapseArrayView::Weights::const_iterator> SynapseArrayView::get_weights()
    const
{
	return {m_weights.begin(), m_weights.end()};
}

boost::iterator_range<SynapseArrayView::Labels::const_iterator> SynapseArrayView::get_labels() const
{
	return {m_labels.begin(), m_labels.end()};
}

SynapseArrayView::Synram const& SynapseArrayView::get_synram() const
{
	return m_synram;
}

std::vector<Port> SynapseArrayView::inputs() const
{
	Port const port(1, ConnectionType::SynapseInputLabel);
	std::set<halco::hicann_dls::vx::v1::SynapseDriverOnSynapseDriverBlock> drivers;
	std::for_each(m_rows.begin(), m_rows.end(), [&](auto const& r) {
		drivers.insert(r.toSynapseDriverOnSynapseDriverBlock());
	});
	std::vector<Port> ret;
	ret.insert(ret.begin(), drivers.size(), port);
	return ret;
}

Port SynapseArrayView::output() const
{
	return Port(m_columns.size(), ConnectionType::SynapticInput);
}

std::ostream& operator<<(std::ostream& os, SynapseArrayView const& config)
{
	os << "SynapseArrayView(size: [rows: " << config.m_rows.size()
	   << ", columns: " << config.m_columns.size() << "])";
	return os;
}

} // namespace grenade::vx::vertex
