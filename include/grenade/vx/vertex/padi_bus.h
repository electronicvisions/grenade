#pragma once
#include <array>
#include <optional>
#include <ostream>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "halco/hicann-dls/vx/padi.h"
#include "hate/visibility.h"

namespace grenade::vx {

struct PortRestriction;

namespace vertex {

struct CrossbarNode;

/**
 * PADI bus connecting a set of crossbar nodes to a set of synapse drivers.
 */
struct PADIBus
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef halco::hicann_dls::vx::PADIBusOnDLS Coordinate;

	/**
	 * Construct PADI bus at specified location.
	 * @param coordinate Location
	 */
	PADIBus(Coordinate const& coordinate) SYMBOL_VISIBLE;

	constexpr static bool variadic_input = true;
	constexpr std::array<Port, 1> inputs() const
	{
		return {Port(1, ConnectionType::CrossbarOutputLabel)};
	}

	constexpr Port output() const
	{
		return Port(1, ConnectionType::SynapseDriverInputLabel);
	}

	bool supports_input_from(
	    CrossbarNode const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

	Coordinate const& get_coordinate() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, PADIBus const& config) SYMBOL_VISIBLE;

private:
	Coordinate m_coordinate;
};

} // vertex

} // grenade::vx
