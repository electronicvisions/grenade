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

SynapseRowViewHandle::UnsignedRow SynapseRowViewHandle::get_causal_correlation() const
{
	return m_synapses.get_causal_correlation(m_row);
}

SynapseRowViewHandle::UnsignedRow SynapseRowViewHandle::get_acausal_correlation() const
{
	return m_synapses.get_acausal_correlation(m_row);
}

SynapseRowViewHandle::CorrelationRow SynapseRowViewHandle::get_correlation(bool const reset) const
{
	return m_synapses.get_correlation(m_row, reset);
}

void SynapseRowViewHandle::reset_correlation() const
{
	m_synapses.reset_correlation(m_row);
}


SynapseRowViewHandleSigned::SignedRow SynapseRowViewHandleSigned::get_weights() const
{
	return static_cast<SignedRow>(m_exc.get_weights()) -
	       static_cast<SignedRow>(m_inh.get_weights());
}

void SynapseRowViewHandleSigned::set_weights(SignedRow const& value) const
{
	m_exc.set_weights(static_cast<UnsignedRow>(libnux::vx::saturate_weight(value)));
	m_inh.set_weights(static_cast<UnsignedRow>(libnux::vx::saturate_weight(-value)));
}

SynapseRowViewHandleSigned::UnsignedRow SynapseRowViewHandleSigned::get_causal_correlation(
    CorrelationSource const source) const
{
	switch (source) {
		case CorrelationSource::exc: {
			return m_exc.get_causal_correlation();
		}
		case CorrelationSource::inh: {
			return m_inh.get_causal_correlation();
		}
		default: {
		}
	}
	exit(1);
}

SynapseRowViewHandleSigned::UnsignedRow SynapseRowViewHandleSigned::get_acausal_correlation(
    CorrelationSource const source) const
{
	switch (source) {
		case CorrelationSource::exc: {
			return m_exc.get_acausal_correlation();
		}
		case CorrelationSource::inh: {
			return m_inh.get_acausal_correlation();
		}
		default: {
		}
	}
	exit(1);
}

SynapseRowViewHandleSigned::CorrelationRow SynapseRowViewHandleSigned::get_correlation(
    bool const reset, CorrelationSource const source) const
{
	switch (source) {
		case CorrelationSource::exc: {
			return m_exc.get_correlation(reset);
		}
		case CorrelationSource::inh: {
			return m_inh.get_correlation(reset);
		}
		default: {
		}
	}
	exit(1);
}

void SynapseRowViewHandleSigned::reset_correlation(CorrelationSource const source) const
{
	switch (source) {
		case CorrelationSource::exc: {
			m_exc.reset_correlation();
			return;
		}
		case CorrelationSource::inh: {
			m_inh.reset_correlation();
			return;
		}
		default: {
		}
	}
	exit(1);
}

} // namespace grenade::vx::ppu
