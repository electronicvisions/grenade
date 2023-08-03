#pragma once
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/port.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "hate/visibility.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow {

struct PortRestriction;

namespace vertex {

struct ExternalInput;

/**
 * Input from the FPGA to the Crossbar.
 * Since the data to the individual channels is split afterwards, they are presented merged here.
 */
struct CrossbarL2Input : public EntityOnChip
{
	constexpr static bool can_connect_different_execution_instances = false;

	/**
	 * Construct CrossbarL2Input.
	 * @param chip_coordinate Coordinate of chip to use
	 */
	CrossbarL2Input(ChipCoordinate const& chip_coordinate = ChipCoordinate()) SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	constexpr std::array<Port, 1> inputs() const
	{
		return {Port(1, ConnectionType::TimedSpikeToChipSequence)};
	}

	constexpr Port output() const
	{
		return Port(1, ConnectionType::CrossbarInputLabel);
	}

	friend std::ostream& operator<<(std::ostream& os, CrossbarL2Input const& config) SYMBOL_VISIBLE;

	bool operator==(CrossbarL2Input const& other) const SYMBOL_VISIBLE;
	bool operator!=(CrossbarL2Input const& other) const SYMBOL_VISIBLE;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex

} // grenade::vx::signal_flow
