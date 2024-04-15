#pragma once
#include "grenade/vx/ppu/synapse_row_view_handle.h"

#include "libnux/vx/vector_row_math.h"

namespace grenade::vx::ppu {

template <typename Row>
Row SynapseRowViewHandle::get_weights() const
{
	return static_cast<Row>(m_synapses.get_weights(m_row));
}

template <typename Row>
void SynapseRowViewHandle::set_weights(Row const& value, bool const saturate) const
{
	m_synapses.set_weights(
	    static_cast<SynapseArrayViewHandle::Row>(
	        saturate ? libnux::vx::saturate_weight(value) : value),
	    m_row);
}

} // namespace grenade::vx::ppu
