#pragma once
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/port.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "haldls/vx/v3/event.h"
#include "haldls/vx/v3/madc.h"
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
 * Readout of neuron voltages via the MADC.
 */
struct MADCReadoutView
{
	constexpr static bool can_connect_different_execution_instances = false;

	struct Source
	{
		typedef halco::hicann_dls::vx::v3::AtomicNeuronOnDLS Coord;
		Coord coord{};
		typedef lola::vx::v3::AtomicNeuron::Readout::Source Type;
		Type type{};

		bool operator==(Source const& other) const SYMBOL_VISIBLE;
		bool operator!=(Source const& other) const SYMBOL_VISIBLE;

		template <typename Archive>
		void serialize(Archive& ar, std::uint32_t);
	};

	/**
	 * Configuration of (timed) selection between the two MADC channels.
	 */
	struct SourceSelection
	{
		/** Initially selected channel. */
		typedef halco::hicann_dls::vx::SourceMultiplexerOnReadoutSourceSelection Initial;
		Initial initial{};

		/** Period with which to switch between channels. */
		typedef haldls::vx::v3::MADCConfig::ActiveMuxInputSelectLength Period;
		Period period{};

		bool operator==(SourceSelection const& other) const SYMBOL_VISIBLE;
		bool operator!=(SourceSelection const& other) const SYMBOL_VISIBLE;

		template <typename Archive>
		void serialize(Archive& ar, std::uint32_t);
	};

	MADCReadoutView() = default;

	/**
	 * Construct MADCReadoutView.
	 * @param first_source Neuron source and location to read out
	 * @param second_source Optional second neuron source and location to read out
	 * @param source_selection Source selection config
	 */
	explicit MADCReadoutView(
	    Source const& first_source,
	    std::optional<Source> const& second_source,
	    SourceSelection const& source_selection) SYMBOL_VISIBLE;

	Source const& get_first_source() const SYMBOL_VISIBLE;
	std::optional<Source> const& get_second_source() const SYMBOL_VISIBLE;
	SourceSelection const& get_source_selection() const SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	std::vector<Port> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, MADCReadoutView const& config) SYMBOL_VISIBLE;

	bool supports_input_from(
	    NeuronView const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

	bool operator==(MADCReadoutView const& other) const SYMBOL_VISIBLE;
	bool operator!=(MADCReadoutView const& other) const SYMBOL_VISIBLE;

private:
	Source m_first_source{};
	std::optional<Source> m_second_source{};
	SourceSelection m_source_selection{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex

} // grenade::vx::signal_flow
