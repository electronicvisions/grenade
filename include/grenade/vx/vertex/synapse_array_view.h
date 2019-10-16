#pragma once
#include <array>
#include <stddef.h>
#include <vector>
#include <boost/range/iterator_range.hpp>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "haldls/vx/synapse_driver.h"
#include "hate/visibility.h"
#include "lola/vx/synapse.h"

namespace halco::hicann_dls::vx {
struct SynapseRowOnSynram;
} // namespace halco::hicann_dls::vx

namespace grenade::vx::vertex {

/**
 * A rectangular (not necessarily consecutive) view of synapses.
 * TODO: split out synapse rows
 */
struct SynapseArrayView
{
	constexpr static bool can_connect_different_execution_instances = false;
	constexpr static bool single_outgoing_vertex = true;

	struct SynapseRow
	{
		typedef lola::vx::SynapseMatrix::Weight Weight;

		haldls::vx::SynapseDriverConfig::RowMode mode;

		halco::hicann_dls::vx::SynapseRowOnSynram coordinate;

		std::vector<Weight> weights;
	};

	typedef std::vector<SynapseRow> synapse_rows_type;

	SynapseArrayView() SYMBOL_VISIBLE;

	explicit SynapseArrayView(synapse_rows_type const& synapse_rows) SYMBOL_VISIBLE;

	size_t num_sends;

	/**
	 * Accessor to synapse row configuration via a range.
	 * @return Range of synapse row configuration
	 */
	boost::iterator_range<synapse_rows_type::const_iterator> synapse_rows() const SYMBOL_VISIBLE;

	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

private:
	synapse_rows_type m_synapse_rows;
};

} // grenade::vx::vertex
