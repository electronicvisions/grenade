#pragma once
#include <ostream>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/synapse.h"
#include "halco/hicann-dls/vx/synapse_driver.h"
#include "haldls/vx/synapse_driver.h"
#include "hate/visibility.h"

namespace grenade::vx {

struct PortRestriction;

namespace vertex {

struct PADIBus;

/**
 * Synapse driver.
 */
struct SynapseDriver
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef haldls::vx::SynapseDriverConfig::RowAddressCompareMask Config;
	typedef halco::hicann_dls::vx::SynapseDriverOnDLS Coordinate;
	typedef halco::common::typed_array<
	    haldls::vx::SynapseDriverConfig::RowMode,
	    halco::hicann_dls::vx::SynapseRowOnSynapseDriver>
	    RowModes;

	/**
	 * Construct synapse driver at specified location with specified configuration.
	 * @param coordinate Location
	 * @param config Configuration
	 */
	SynapseDriver(Coordinate const& coordinate, Config const& config, RowModes const& row_modes)
	    SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	constexpr std::array<Port, 1> inputs() const
	{
		return {Port(1, ConnectionType::SynapseDriverInputLabel)};
	}

	constexpr Port output() const
	{
		return Port(1, ConnectionType::SynapseInputLabel);
	}

	bool supports_input_from(
	    PADIBus const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

	Coordinate get_coordinate() const SYMBOL_VISIBLE;
	Config get_config() const SYMBOL_VISIBLE;
	RowModes get_row_modes() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, SynapseDriver const& config) SYMBOL_VISIBLE;

private:
	Coordinate m_coordinate;
	Config m_config;
	RowModes m_row_modes;
};

} // vertex

} // grenade::vx
