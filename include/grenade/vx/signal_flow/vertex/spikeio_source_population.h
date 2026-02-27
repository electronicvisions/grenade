#pragma once
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/port.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/fpga.h"
#include "haldls/vx/fpga.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <map>

namespace cereal {
struct access;
}

namespace grenade::vx::signal_flow::vertex {

struct SpikeIOSourcePopulation : public EntityOnChip
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef haldls::vx::SpikeIOConfig Config;
	typedef std::map<halco::hicann_dls::vx::SpikeLabel, halco::hicann_dls::vx::SpikeIOAddress>
	    OutputRouting;
	typedef std::map<halco::hicann_dls::vx::SpikeIOAddress, halco::hicann_dls::vx::SpikeLabel>
	    InputRouting;

	SpikeIOSourcePopulation() = default;

	SpikeIOSourcePopulation(
	    Config const& config,
	    OutputRouting const& out_rt,
	    InputRouting const& in_rt,
	    ChipOnExecutor const& chip_on_executor = ChipOnExecutor()) SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	constexpr std::array<Port, 0> inputs() const
	{
		return {};
	}

	Port output() const SYMBOL_VISIBLE;

	Config const& get_config() const SYMBOL_VISIBLE;
	OutputRouting const& get_output_routing() const SYMBOL_VISIBLE;
	InputRouting const& get_input_routing() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, SpikeIOSourcePopulation const& v)
	    SYMBOL_VISIBLE;

	bool operator==(SpikeIOSourcePopulation const& other) const SYMBOL_VISIBLE;
	bool operator!=(SpikeIOSourcePopulation const& other) const SYMBOL_VISIBLE;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);

	Config m_config{};
	OutputRouting m_output{};
	InputRouting m_input{};
};

} // namespace grenade::vx::signal_flow::vertex
