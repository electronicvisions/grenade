#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/population_on_execution_instance.h"
#include "grenade/vx/network/projection_on_execution_instance.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
#include "halco/common/geometry.h"
#include "hate/visibility.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Plasticity rule.
 */
struct GENPYBIND(visible) PlasticityRule
{
	/**
	 * Descriptor to projections this rule has access to.
	 * All projections are required to be dense and in order.
	 */
	std::vector<ProjectionOnExecutionInstance> projections{};

	/**
	 * Population handle parameters.
	 */
	struct PopulationHandle
	{
		typedef lola::vx::v3::AtomicNeuron::Readout::Source NeuronReadoutSource;

		/**
		 * Descriptor of population.
		 */
		PopulationOnExecutionInstance descriptor;
		/**
		 * Readout source specification per neuron circuit used for static configuration such that
		 * the plasticity rule can read the specified signal.
		 */
		std::vector<std::map<
		    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
		    std::vector<std::optional<NeuronReadoutSource>>>>
		    neuron_readout_sources;

		bool operator==(PopulationHandle const& other) const SYMBOL_VISIBLE;
		bool operator!=(PopulationHandle const& other) const SYMBOL_VISIBLE;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, PopulationHandle const& population)
		    SYMBOL_VISIBLE;
	};
	std::vector<PopulationHandle> populations;

	/**
	 * Plasticity rule kernel to be compiled into the PPU program.
	 */
	std::string kernel{};

	/**
	 * Timing information for execution of the rule.
	 */
	struct Timer
	{
		/** PPU clock cycles. */
		struct GENPYBIND(inline_base("*")) Value
		    : public halco::common::detail::BaseType<Value, uint64_t>
		{
			constexpr explicit Value(uint64_t const value = 0) : base_t(value) {}
		};

		Value start;
		Value period;
		size_t num_periods;

		bool operator==(Timer const& other) const SYMBOL_VISIBLE;
		bool operator!=(Timer const& other) const SYMBOL_VISIBLE;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Timer const& timer) SYMBOL_VISIBLE;
	} timer;


	/**
	 * Enable whether this plasticity rule requires all projections to have one source per row and
	 * them being in order.
	 */
	bool enable_requires_one_source_per_row_in_order{false};

	/*
	 * Recording information for execution of the rule.
	 * Raw recording of one scratchpad memory region for all timed invocations of the
	 * rule. No automated recording of time is performed.
	 */
	typedef signal_flow::vertex::PlasticityRule::RawRecording RawRecording GENPYBIND(opaque(false));

	/**
	 * Recording information for execution of the rule.
	 * Recording of exclusive scratchpad memory per rule invocation with
	 * time recording and returned data as time-annotated events.
	 */
	typedef signal_flow::vertex::PlasticityRule::TimedRecording TimedRecording
	    GENPYBIND(opaque(false));

	/**
	 * Recording memory provided to plasticity rule kernel and recorded after
	 * execution.
	 */
	typedef signal_flow::vertex::PlasticityRule::Recording Recording;
	std::optional<Recording> recording;

	/**
	 * Identifier of same plasticity rule.
	 * Plasticity rules with equal identifier share their state across realtime snippets.
	 * Currently, in addition, the complete provided kernel code is required to be equal.
	 */
	typedef signal_flow::vertex::PlasticityRule::ID ID GENPYBIND(opaque(false));
	ID id;

	/**
	 * Recording data corresponding to a raw recording.
	 */
	typedef signal_flow::vertex::PlasticityRule::RawRecordingData RawRecordingData
	    GENPYBIND(opaque(false));

	/**
	 * Extracted recorded data of observables corresponding to timed recording.
	 */
	struct TimedRecordingData
	{
		/**
		 * Recording per logical synapse, where for each sample the outer dimension of the data are
		 * the logical synapses of the projection and the inner dimension are the performed
		 * recordings of the corresponding hardware synapse(s).
		 */
		typedef std::variant<
		    std::vector<common::TimedDataSequence<std::vector<std::vector<int8_t>>>>,
		    std::vector<common::TimedDataSequence<std::vector<std::vector<uint8_t>>>>,
		    std::vector<common::TimedDataSequence<std::vector<std::vector<int16_t>>>>,
		    std::vector<common::TimedDataSequence<std::vector<std::vector<uint16_t>>>>>
		    EntryPerSynapse;

		/**
		 * Recording per logical neuron, where for each sample the outer dimension of the data are
		 * the logical neurons of the population and the inner dimensions are the performed
		 * recordings of the corresponding hardware neuron(s) per compartment.
		 */
		typedef std::variant<
		    std::vector<common::TimedDataSequence<std::vector<std::map<
		        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
		        std::vector<int8_t>>>>>,
		    std::vector<common::TimedDataSequence<std::vector<std::map<
		        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
		        std::vector<uint8_t>>>>>,
		    std::vector<common::TimedDataSequence<std::vector<std::map<
		        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
		        std::vector<int16_t>>>>>,
		    std::vector<common::TimedDataSequence<std::vector<std::map<
		        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
		        std::vector<uint16_t>>>>>>
		    EntryPerNeuron;

		typedef signal_flow::vertex::PlasticityRule::TimedRecordingData::Entry EntryArray;

		std::map<std::string, std::map<ProjectionOnExecutionInstance, EntryPerSynapse>>
		    data_per_synapse;
		std::map<std::string, std::map<PopulationOnExecutionInstance, EntryPerNeuron>>
		    data_per_neuron;
		std::map<std::string, EntryArray> data_array;
	};

	/**
	 * Recorded data.
	 */
	typedef std::variant<RawRecordingData, TimedRecordingData> RecordingData;

	PlasticityRule() = default;

	bool operator==(PlasticityRule const& other) const SYMBOL_VISIBLE;
	bool operator!=(PlasticityRule const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, PlasticityRule const& plasticity_rule)
	    SYMBOL_VISIBLE;
};


typedef common::TimedData<std::vector<std::vector<int8_t>>> _SingleEntryPerSynapseInt8
    GENPYBIND(opaque(false), expose_as(_SingleEntryPerSynapseInt8));
typedef common::TimedData<std::vector<std::vector<uint8_t>>> _SingleEntryPerSynapseUInt8
    GENPYBIND(opaque(false), expose_as(_SingleEntryPerSynapseUInt8));
typedef common::TimedData<std::vector<std::vector<int16_t>>> _SingleEntryPerSynapseInt16
    GENPYBIND(opaque(false), expose_as(_SingleEntryPerSynapseInt16));
typedef common::TimedData<std::vector<std::vector<uint16_t>>> _SingleEntryPerSynapseUInt16
    GENPYBIND(opaque(false), expose_as(_SingleEntryPerSynapseUInt16));
typedef common::TimedData<std::vector<int8_t>> _ArrayEntryInt8
    GENPYBIND(opaque(false), expose_as(_ArrayEntryInt8));
typedef common::TimedData<std::vector<uint8_t>> _ArrayEntryUInt8
    GENPYBIND(opaque(false), expose_as(_ArrayEntryUInt8));
typedef common::TimedData<std::vector<int16_t>> _ArrayEntryInt16
    GENPYBIND(opaque(false), expose_as(_ArrayEntryInt16));
typedef common::TimedData<std::vector<uint16_t>> _ArrayEntryUInt16
    GENPYBIND(opaque(false), expose_as(_ArrayEntryUInt16));

typedef common::TimedData<std::vector<
    std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, std::vector<int8_t>>>>
    _SingleEntryPerNeuronInt8 GENPYBIND(opaque(false), expose_as(_SingleEntryPerNeuronInt8));
typedef common::TimedData<std::vector<
    std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, std::vector<uint8_t>>>>
    _SingleEntryPerNeuronUInt8 GENPYBIND(opaque(false), expose_as(_SingleEntryPerNeuronUInt8));
typedef common::TimedData<std::vector<
    std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, std::vector<int16_t>>>>
    _SingleEntryPerNeuronInt16 GENPYBIND(opaque(false), expose_as(_SingleEntryPerNeuronInt16));
typedef common::TimedData<std::vector<
    std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, std::vector<uint16_t>>>>
    _SingleEntryPerNeuronUInt16 GENPYBIND(opaque(false), expose_as(_SingleEntryPerNeuronUInt16));

} // namespace network
} // namespace grenade::vx
