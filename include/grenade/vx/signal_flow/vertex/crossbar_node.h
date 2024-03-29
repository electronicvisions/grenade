#pragma once
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/port.h"
#include "grenade/vx/signal_flow/port_restriction.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/routing_crossbar.h"
#include "haldls/vx/v3/routing_crossbar.h"
#include "hate/visibility.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex {

struct BackgroundSpikeSource;

/**
 * Crossbar node.
 */
struct CrossbarNode : public EntityOnChip
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef haldls::vx::v3::CrossbarNode Config;
	typedef haldls::vx::v3::CrossbarNode::coordinate_type Coordinate;

	CrossbarNode() = default;

	/**
	 * Construct node at specified location with specified configuration.
	 * @param coordinate Location
	 * @param config Configuration
	 * @param chip_coordinate Coordinate of chip to use
	 */
	CrossbarNode(
	    Coordinate const& coordinate,
	    Config const& config,
	    ChipCoordinate const& chip_coordinate = ChipCoordinate()) SYMBOL_VISIBLE;

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

	bool supports_input_from(
	    BackgroundSpikeSource const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

private:
	Coordinate m_coordinate{};
	Config m_config{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // grenade::vx::signal_flow::vertex
