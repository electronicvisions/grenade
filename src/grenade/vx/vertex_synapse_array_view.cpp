#include "grenade/vx/vertex/synapse_array_view.h"

#include <stdexcept>
#include "halco/hicann-dls/vx/synapse.h"

namespace grenade::vx::vertex {

SynapseArrayView::SynapseArrayView() : num_sends(1), m_synapse_rows() {}

SynapseArrayView::SynapseArrayView(synapse_rows_type const& synapse_rows) :
    num_sends(1), m_synapse_rows(synapse_rows)
{
	for (auto const& row : m_synapse_rows) {
		if (row.weights.size() != output().size) {
			throw std::runtime_error("Weight row size does not match output size.");
		}
	}
}

boost::iterator_range<SynapseArrayView::synapse_rows_type::const_iterator>
SynapseArrayView::synapse_rows() const
{
	return {m_synapse_rows.begin(), m_synapse_rows.end()};
}

std::array<Port, 1> SynapseArrayView::inputs() const
{
	return {Port(m_synapse_rows.size(), ConnectionType::SynapseInputLabel)};
}

Port SynapseArrayView::output() const
{
	return Port(m_synapse_rows.at(0).weights.size(), ConnectionType::SynapticInput);
}

} // namespace grenade::vx::vertex
