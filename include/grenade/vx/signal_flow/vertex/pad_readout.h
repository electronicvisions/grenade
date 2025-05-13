#pragma once
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/port.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/readout.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <tuple>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow {

struct PortRestriction;

namespace vertex {

struct NeuronView;

/**
 * Readout of neuron voltages via a pad.
 */
struct PadReadoutView : public EntityOnChip
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef halco::hicann_dls::vx::v3::PadOnDLS Coordinate;

	struct Source
	{
		typedef halco::hicann_dls::vx::v3::AtomicNeuronOnDLS Coord;
		Coord coord{};
		typedef lola::vx::v3::AtomicNeuron::Readout::Source Type;
		Type type{};
		bool enable_buffered{false};

		bool operator==(Source const& other) const SYMBOL_VISIBLE;
		bool operator!=(Source const& other) const SYMBOL_VISIBLE;

		template <typename Archive>
		void serialize(Archive& ar, std::uint32_t);
	};

	PadReadoutView() = default;

	/**
	 * Construct PadReadoutView.
	 * @param source Neuron source and location to read out
	 * @param coordinate Pad to use
	 * @param chip_on_executor Coordinate of chip to use
	 */
	explicit PadReadoutView(
	    Source const& source,
	    Coordinate const& coordinate,
	    ChipOnExecutor const& chip_on_executor = ChipOnExecutor()) SYMBOL_VISIBLE;

	Source const& get_source() const SYMBOL_VISIBLE;
	Coordinate const& get_coordinate() const SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, PadReadoutView const& config) SYMBOL_VISIBLE;

	bool supports_input_from(
	    NeuronView const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

	bool operator==(PadReadoutView const& other) const SYMBOL_VISIBLE;
	bool operator!=(PadReadoutView const& other) const SYMBOL_VISIBLE;

private:
	Source m_source{};
	Coordinate m_coordinate{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex

} // grenade::vx::signal_flow
