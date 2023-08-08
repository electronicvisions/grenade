#pragma once
#include "grenade/vx/signal_flow/port.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "halco/hicann-dls/vx/v3/event.h"
#include "haldls/vx/v3/event.h"
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::network::vertex::transformation {

/**
 * Transformation merging (translated) spike-trains from external user-input or other execution
 * instance(s). It introduces one additional copy of all forwarded data and sorting the forwarded
 * data at once over time exactly if more than one input is to be merged.
 */
struct SYMBOL_VISIBLE ExternalSourceMerger : public signal_flow::vertex::Transformation::Function
{
	~ExternalSourceMerger() SYMBOL_VISIBLE;

	ExternalSourceMerger() = default;

	/**
	 * Input from another execution instance with required label translation.
	 */
	struct InterExecutionInstanceInput
	{
		constexpr static signal_flow::ConnectionType type =
		    signal_flow::ConnectionType::TimedSpikeFromChipSequence;

		/**
		 * Post labels for pre label.
		 */
		std::map<
		    halco::hicann_dls::vx::v3::SpikeLabel,
		    std::vector<halco::hicann_dls::vx::v3::SpikeLabel>>
		    translation;

		bool operator==(InterExecutionInstanceInput const& other) const;
	};

	/**
	 * User-supplied external input.
	 */
	struct ExternalInput
	{
		constexpr static signal_flow::ConnectionType type =
		    signal_flow::ConnectionType::TimedSpikeToChipSequence;

		bool operator==(ExternalInput const& other) const;
	};

	typedef std::variant<InterExecutionInstanceInput, ExternalInput> Input;

	/**
	 * Construct merger for spike-trains of external source.
	 * @param inputs Input configuration.
	 */
	ExternalSourceMerger(std::vector<Input> const& inputs) SYMBOL_VISIBLE;

	std::vector<signal_flow::Port> inputs() const SYMBOL_VISIBLE;
	signal_flow::Port output() const SYMBOL_VISIBLE;

	bool equal(signal_flow::vertex::Transformation::Function const& other) const SYMBOL_VISIBLE;

	Value apply(std::vector<Value> const& value) const SYMBOL_VISIBLE;

private:
	std::vector<Input> m_inputs;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::network::vertex::transformation
