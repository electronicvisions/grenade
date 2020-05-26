#pragma once
#include <array>
#include <ostream>
#include <stddef.h>
#include <vector>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "halco/hicann-dls/vx/cadc.h"
#include "hate/visibility.h"


namespace grenade::vx::vertex {

struct NeuronView;

/**
 * Readout of membrane voltages via the CADC.
 */
struct CADCMembraneReadoutView
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef std::vector<halco::hicann_dls::vx::SynapseOnSynapseRow> Columns;
	typedef halco::hicann_dls::vx::SynramOnDLS Synram;

	/**
	 * Construct CADCMembraneReadoutView with specified size.
	 * @param columns Columns to read out
	 */
	explicit CADCMembraneReadoutView(Columns const& columns, Synram const& synram) SYMBOL_VISIBLE;

	Columns const& get_columns() const SYMBOL_VISIBLE;
	Synram const& get_synram() const SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, CADCMembraneReadoutView const& config)
	    SYMBOL_VISIBLE;

	bool supports_input_from(NeuronView const& input) const SYMBOL_VISIBLE;

private:
	Columns m_columns;
	Synram m_synram;
};

} // grenade::vx::vertex
