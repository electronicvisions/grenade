#pragma once
#include "grenade/vx/ppu/synapse_row_view_handle_range.h"

namespace grenade::vx::ppu {

inline SynapseRowViewHandleRange::Iterator::Iterator(SynapseArrayViewHandle const& synapses) :
    m_synapses(synapses), m_row(0)
{
	while (m_row < m_synapses.rows.size && !m_synapses.rows.test(m_row)) {
		m_row++;
	}
}

inline SynapseRowViewHandleRange::Iterator::Iterator(
    SynapseArrayViewHandle const& synapses, size_t row) :
    m_synapses(synapses), m_row(row)
{}

inline bool SynapseRowViewHandleRange::Iterator::equal(Iterator const& other) const
{
	return m_row == other.m_row;
}

inline void SynapseRowViewHandleRange::Iterator::increment()
{
	do {
		m_row++;
	} while (m_row < m_synapses.rows.size && !m_synapses.rows.test(m_row));
}

inline SynapseRowViewHandleRange::Iterator::reference
SynapseRowViewHandleRange::Iterator::dereference() const
{
	return SynapseRowViewHandleRange::Iterator::reference(m_synapses, m_row);
}

inline SynapseRowViewHandleRange::SynapseRowViewHandleRange(
    SynapseArrayViewHandle const& synapses) :
    m_begin(synapses), m_end(synapses, synapses.rows.size)
{}

inline SynapseRowViewHandleRange::Iterator SynapseRowViewHandleRange::begin() const
{
	return m_begin;
}

inline SynapseRowViewHandleRange::Iterator SynapseRowViewHandleRange::end() const
{
	return m_end;
}


inline SynapseRowViewHandleSignedRange::Iterator::Iterator(
    SynapseArrayViewHandle const& synapses_exc, SynapseArrayViewHandle const& synapses_inh) :
    m_synapses_exc(synapses_exc), m_synapses_inh(synapses_inh), m_exc_row(0), m_inh_row(0)
{
	check();
	while (m_exc_row < m_synapses_exc.rows.size && !m_synapses_exc.rows.test(m_exc_row)) {
		m_exc_row++;
	}
	while (m_inh_row < m_synapses_inh.rows.size && !m_synapses_inh.rows.test(m_inh_row)) {
		m_inh_row++;
	}
}

inline SynapseRowViewHandleSignedRange::Iterator::Iterator(
    SynapseArrayViewHandle const& synapses_exc,
    SynapseArrayViewHandle const& synapses_inh,
    size_t exc_row,
    size_t inh_row) :
    m_synapses_exc(synapses_exc),
    m_synapses_inh(synapses_inh),
    m_exc_row(exc_row),
    m_inh_row(inh_row)
{
	check();
}

inline void SynapseRowViewHandleSignedRange::Iterator::check() const
{
	size_t num_active_exc = 0;
	for (size_t i = 0; i < m_synapses_exc.rows.size; ++i) {
		num_active_exc += m_synapses_exc.rows.test(i);
	}
	size_t num_active_inh = 0;
	for (size_t i = 0; i < m_synapses_inh.rows.size; ++i) {
		num_active_inh += m_synapses_inh.rows.test(i);
	}
	if (num_active_exc != num_active_inh) {
		exit(1);
	}
}

inline bool SynapseRowViewHandleSignedRange::Iterator::equal(Iterator const& other) const
{
	return m_exc_row == other.m_exc_row && m_inh_row == other.m_inh_row;
}

inline void SynapseRowViewHandleSignedRange::Iterator::increment()
{
	do {
		m_exc_row++;
	} while (m_exc_row < m_synapses_exc.rows.size && !m_synapses_exc.rows.test(m_exc_row));
	do {
		m_inh_row++;
	} while (m_inh_row < m_synapses_inh.rows.size && !m_synapses_inh.rows.test(m_inh_row));
}

inline SynapseRowViewHandleSignedRange::Iterator::reference
SynapseRowViewHandleSignedRange::Iterator::dereference() const
{
	return SynapseRowViewHandleSignedRange::Iterator::reference(
	    SynapseRowViewHandle(m_synapses_exc, m_exc_row),
	    SynapseRowViewHandle(m_synapses_inh, m_inh_row));
}

inline SynapseRowViewHandleSignedRange::SynapseRowViewHandleSignedRange(
    SynapseArrayViewHandle const& synapses_exc, SynapseArrayViewHandle const& synapses_inh) :
    m_begin(synapses_exc, synapses_inh),
    m_end(synapses_exc, synapses_inh, synapses_exc.rows.size, synapses_inh.rows.size)
{}

inline SynapseRowViewHandleSignedRange::Iterator SynapseRowViewHandleSignedRange::begin() const
{
	return m_begin;
}

inline SynapseRowViewHandleSignedRange::Iterator SynapseRowViewHandleSignedRange::end() const
{
	return m_end;
}


inline SynapseRowViewHandleRange make_synapse_row_view_handle_range(
    std::array<SynapseArrayViewHandle, 1> const& synapses)
{
	return SynapseRowViewHandleRange(synapses[0]);
}


inline SynapseRowViewHandleSignedRange make_synapse_row_view_handle_range(
    std::array<SynapseArrayViewHandle, 2> const& synapses)
{
	return SynapseRowViewHandleSignedRange(synapses[0], synapses[1]);
}

} // namespace grenade::vx::ppu
