#pragma once
#include <array>
#include <optional>
#include <ostream>
#include <stddef.h>
#include <vector>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "halco/hicann-dls/vx/neuron.h"
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

	typedef std::vector<halco::hicann_dls::vx::NeuronColumnOnDLS> Columns;
	typedef halco::hicann_dls::vx::NeuronRowOnDLS Row;

	/**
	 * Construct NeuronView with specified neurons.
	 * @param columns Neuron columns
	 * @param row Neuron row
	 */
	explicit NeuronView(Columns const& columns, Row const& row) SYMBOL_VISIBLE;

	Columns const& get_columns() const SYMBOL_VISIBLE;
	Row const& get_row() const SYMBOL_VISIBLE;

	constexpr static bool variadic_input = true;
	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, NeuronView const& config) SYMBOL_VISIBLE;

	bool supports_input_from(
	    SynapseArrayView const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

private:
	Columns m_columns;
	Row m_row;
};

} // vertex

} // grenade::vx
