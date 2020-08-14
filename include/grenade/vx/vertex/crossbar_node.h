#pragma once
#include <ostream>
#include <stddef.h>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "halco/hicann-dls/vx/routing_crossbar.h"
#include "haldls/vx/v1/routing_crossbar.h"
#include "hate/visibility.h"

namespace grenade::vx::vertex {

/**
 * Crossbar node.
 */
struct CrossbarNode
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef haldls::vx::v1::CrossbarNode Config;
	typedef haldls::vx::v1::CrossbarNode::coordinate_type Coordinate;

	/**
	 * Construct node at specified location with specified configuration.
	 * @param coordinate Location
	 * @param config Configuration
	 */
	CrossbarNode(Coordinate const& coordinate, Config const& config) SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	constexpr std::array<Port, 1> inputs() const
	{
		return {Port(1, ConnectionType::CrossbarInputLabel)};
	}

	constexpr Port output() const
	{
		return Port(1, ConnectionType::CrossbarOutputLabel);
	}

	Coordinate const& get_coordinate() const SYMBOL_VISIBLE;
	Config const& get_config() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, CrossbarNode const& config) SYMBOL_VISIBLE;

private:
	Coordinate m_coordinate;
	Config m_config;
};

} // grenade::vx::vertex
