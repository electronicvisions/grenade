#pragma once
#include <array>
#include <ostream>
#include <stddef.h>
#include <vector>
#include <boost/range/iterator_range.hpp>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "hate/visibility.h"
#include "lola/vx/v2/synapse.h"

namespace grenade::vx::vertex {

/**
 * A rectangular view of synapses connected to a set of synapse drivers.
 */
struct SynapseArrayView
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef std::vector<halco::hicann_dls::vx::v2::SynapseRowOnSynram> Rows;
	typedef halco::hicann_dls::vx::v2::SynramOnDLS Synram;
	typedef std::vector<halco::hicann_dls::vx::v2::SynapseOnSynapseRow> Columns;
	typedef std::vector<std::vector<lola::vx::v2::SynapseMatrix::Label>> Labels;
	typedef std::vector<std::vector<lola::vx::v2::SynapseMatrix::Weight>> Weights;

	/**
	 * Construct synapse array view.
	 * @param synram Synram location of synapses
	 * @param rows Coordinates of rows
	 * @param columns Coordinates of columns
	 * @param weights Weight values
	 * @param labels Label values
	 */
	template <
	    typename SynramT,
	    typename RowsT,
	    typename ColumnsT,
	    typename WeightsT,
	    typename LabelsT>
	explicit SynapseArrayView(
	    SynramT&& synram, RowsT&& rows, ColumnsT&& columns, WeightsT&& weights, LabelsT&& labels);

	/**
	 * Accessor to synapse row coordinates via a range.
	 * @return Range of synapse row coordinates
	 */
	boost::iterator_range<Rows::const_iterator> get_rows() const SYMBOL_VISIBLE;

	/**
	 * Accessor to synapse column coordinates via a range.
	 * @return Range of synapse column coordinates
	 */
	boost::iterator_range<Columns::const_iterator> get_columns() const SYMBOL_VISIBLE;

	/**
	 * Accessor to weight configuration via a range.
	 * @return Range of weight configuration
	 */
	boost::iterator_range<Weights::const_iterator> get_weights() const SYMBOL_VISIBLE;

	/**
	 * Accessor to label configuration via a range.
	 * @return Range of label configuration
	 */
	boost::iterator_range<Labels::const_iterator> get_labels() const SYMBOL_VISIBLE;

	Synram const& get_synram() const SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	std::vector<Port> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, SynapseArrayView const& config)
	    SYMBOL_VISIBLE;

private:
	Synram m_synram;
	Rows m_rows;
	Columns m_columns;
	Weights m_weights;
	Labels m_labels;

	void check(
	    Rows const& rows, Columns const& columns, Weights const& weights, Labels const& labels)
	    SYMBOL_VISIBLE;
};

} // grenade::vx::vertex

#include "grenade/vx/vertex/synapse_array_view.tcc"
