#pragma once
#include "grenade/vx/ppu/synapse_row_view_handle.h"
#include <array>
#include <boost/iterator/iterator_facade.hpp>

namespace grenade::vx::ppu {

/**
 * Iterable range of synapse row handles of a synapse array.
 */
struct SynapseRowViewHandleRange
{
	class Iterator
	    : public boost::iterator_facade<
	          Iterator,
	          SynapseRowViewHandle,
	          boost::forward_traversal_tag,
	          SynapseRowViewHandle>
	{
		friend class SynapseRowViewHandleRange;
		friend class boost::iterator_core_access;

		Iterator(SynapseArrayViewHandle const& synapses, size_t row);

		typedef SynapseRowViewHandle reference;
		bool equal(Iterator const& other) const;
		void increment();
		reference dereference() const;

		SynapseArrayViewHandle const& m_synapses;
		size_t m_row;
	};

	SynapseRowViewHandleRange(SynapseArrayViewHandle const& synapses);

	Iterator begin() const;
	Iterator end() const;

private:
	Iterator m_begin;
	Iterator m_end;
};


/**
 * Iterable range of signed synapse row handles of a synapse array.
 */
struct SynapseRowViewHandleSignedRange
{
	class Iterator
	    : public boost::iterator_facade<
	          Iterator,
	          SynapseRowViewHandleSigned,
	          boost::forward_traversal_tag,
	          SynapseRowViewHandleSigned>
	{
		friend class SynapseRowViewHandleSignedRange;
		friend class boost::iterator_core_access;

		Iterator(
		    SynapseArrayViewHandle const& synapses_exc,
		    SynapseArrayViewHandle const& synapses_inh,
		    size_t row);

		typedef SynapseRowViewHandleSigned reference;
		void check() const;
		bool equal(Iterator const& other) const;
		void increment();
		reference dereference() const;

		SynapseArrayViewHandle const& m_synapses_exc;
		SynapseArrayViewHandle const& m_synapses_inh;
		size_t m_row;
	};

	SynapseRowViewHandleSignedRange(
	    SynapseArrayViewHandle const& synapses_exc, SynapseArrayViewHandle const& synapses_inh);

	Iterator begin() const;
	Iterator end() const;

private:
	Iterator m_begin;
	Iterator m_end;
};


/**
 * Construct synapse row view handle range for one unsigned synapse array view handle.
 * To be used when abstract code is written and function overloading is used to select signed or
 * unsigned view ranges.
 * @param synapses Synapses to use for construction of range
 * @return Constructed range
 */
SynapseRowViewHandleRange make_synapse_row_view_handle_range(
    std::array<SynapseArrayViewHandle, 1> const& synapses);


/**
 * Construct synapse row view handle range for one unsigned synapse array view handle.
 * To be used when abstract code is written and function overloading is used to select signed or
 * unsigned view ranges.
 * @param synapses Synapses to use for construction of range, the first is used as excitatory range,
 * the second as inhibitory range
 * @return Constructed range
 */
SynapseRowViewHandleSignedRange make_synapse_row_view_handle_range(
    std::array<SynapseArrayViewHandle, 2> const& synapses);

} // namespace grenade::vx::ppu

#include "grenade/vx/ppu/synapse_row_view_handle_range.tcc"
