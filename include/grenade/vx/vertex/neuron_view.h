#pragma once
#include <array>
#include <optional>
#include <ostream>
#include <stddef.h>
#include <vector>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "halco/hicann-dls/vx/v2/neuron.h"
#include "hate/visibility.h"

namespace grenade::vx {

struct PortRestriction;

namespace vertex {

struct SynapseArrayView;

/**
 * A view of neuron circuits.
 * TODO: add properties
 */
struct NeuronView
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef std::vector<halco::hicann_dls::vx::v2::NeuronColumnOnDLS> Columns;
	typedef std::vector<bool> EnableResets;
	typedef halco::hicann_dls::vx::v2::NeuronRowOnDLS Row;

	/**
	 * Construct NeuronView with specified neurons.
	 * @param columns Neuron columns
	 * @param enable_resets Enable values for initial reset of the neurons
	 * @param row Neuron row
	 */
	template <typename ColumnsT, typename EnableResetsT, typename RowT>
	explicit NeuronView(ColumnsT&& columns, EnableResetsT&& enable_resets, RowT&& row);

	Columns const& get_columns() const SYMBOL_VISIBLE;
	EnableResets const& get_enable_resets() const SYMBOL_VISIBLE;
	Row const& get_row() const SYMBOL_VISIBLE;

	constexpr static bool variadic_input = true;
	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, NeuronView const& config) SYMBOL_VISIBLE;

	bool supports_input_from(
	    SynapseArrayView const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

private:
	void check(Columns const& columns, EnableResets const& enable_resets) SYMBOL_VISIBLE;

	Columns m_columns;
	EnableResets m_enable_resets;
	Row m_row;
};

} // vertex

} // grenade::vx

#include "grenade/vx/vertex/neuron_view.tcc"
