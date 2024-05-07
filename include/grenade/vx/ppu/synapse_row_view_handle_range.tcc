#pragma once
#include "grenade/vx/ppu/synapse_row_view_handle_range.h"
#include <array>

namespace grenade::vx::ppu {

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
	m_row++;
}

inline SynapseRowViewHandleRange::Iterator::reference
SynapseRowViewHandleRange::Iterator::dereference() const
{
	return SynapseRowViewHandleRange::Iterator::reference(m_synapses, m_row);
}

inline SynapseRowViewHandleRange::SynapseRowViewHandleRange(
    SynapseArrayViewHandle const& synapses) :
    m_begin(synapses, 0), m_end(synapses, synapses.rows.size())
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
    SynapseArrayViewHandle const& synapses_exc,
    SynapseArrayViewHandle const& synapses_inh,
    size_t row) :
    m_synapses_exc(synapses_exc), m_synapses_inh(synapses_inh), m_row(row)
{
	check();
}

inline void SynapseRowViewHandleSignedRange::Iterator::check() const
{
	if (m_synapses_exc.rows.size() != m_synapses_inh.rows.size()) {
		exit(1);
	}
}

inline bool SynapseRowViewHandleSignedRange::Iterator::equal(Iterator const& other) const
{
	return m_row == other.m_row;
}

inline void SynapseRowViewHandleSignedRange::Iterator::increment()
{
	m_row++;
}

inline SynapseRowViewHandleSignedRange::Iterator::reference
SynapseRowViewHandleSignedRange::Iterator::dereference() const
{
	return SynapseRowViewHandleSignedRange::Iterator::reference(
	    SynapseRowViewHandle(m_synapses_exc, m_row), SynapseRowViewHandle(m_synapses_inh, m_row));
}

inline SynapseRowViewHandleSignedRange::SynapseRowViewHandleSignedRange(
    SynapseArrayViewHandle const& synapses_exc, SynapseArrayViewHandle const& synapses_inh) :
    m_begin(synapses_exc, synapses_inh, 0),
    m_end(synapses_exc, synapses_inh, synapses_exc.rows.size())
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
