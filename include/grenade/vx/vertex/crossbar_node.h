#pragma once
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "halco/hicann-dls/vx/v2/routing_crossbar.h"
#include "haldls/vx/v2/routing_crossbar.h"
#include "hate/visibility.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>

namespace cereal {
class access;
} // namespace cereal

namespace grenade::vx::vertex {

/**
 * Crossbar node.
 */
struct CrossbarNode
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef haldls::vx::v2::CrossbarNode Config;
	typedef haldls::vx::v2::CrossbarNode::coordinate_type Coordinate;

	CrossbarNode() = default;

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

	bool operator==(CrossbarNode const& other) const SYMBOL_VISIBLE;
	bool operator!=(CrossbarNode const& other) const SYMBOL_VISIBLE;

private:
	Coordinate m_coordinate{};
	Config m_config{};

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // grenade::vx::vertex
