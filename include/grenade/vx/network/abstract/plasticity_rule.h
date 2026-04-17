#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/population.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * PlasticityRule in topology.
 */
struct SYMBOL_VISIBLE GENPYBIND(
    visible,
    holder_type("std::shared_ptr<grenade::vx::network::abstract::PlasticityRule>")) PlasticityRule
    : public grenade::common::PartitionedVertex
{
	struct Parameterization : public grenade::common::PortData
	{
		Parameterization(std::string kernel);

		std::string kernel;

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual bool is_equal_to(PortData const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;
	};

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	struct Dynamics : public grenade::common::BatchedPortData
	{
		typedef grenade::vx::signal_flow::vertex::PlasticityRule::Timer Timer
		    GENPYBIND(opaque(false));
		Dynamics(Timer const& timer, size_t batch_size);

		/**
		 * Timer describing execution schedule of plasticity rule kernel during experiment runtime.
		 */
		Timer timer;

		virtual size_t batch_size() const override;

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual bool is_equal_to(PortData const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;

	private:
		size_t m_batch_size;
	};

	/**
	 * Dynamics port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) DynamicsPortType
	    : public dapr::EmptyProperty<DynamicsPortType, grenade::common::VertexPortType>
	{};

	struct Results : public grenade::common::BatchedPortData
	{
		/**
		 * Data corresponding to a raw recording.
		 */
		typedef signal_flow::vertex::PlasticityRule::RawRecordingData RawData
		    GENPYBIND(opaque(false));

		/**
		 * Extracted recorded data of observables corresponding to timed recording.
		 */
		struct TimedData
		{
			/**
			 * Entry per logical synapse, where for each sample the outer dimension of the data
			 * are the logical synapses of the projection and the inner dimension are the performed
			 * recordings of the corresponding hardware synapse(s).
			 */
			typedef std::variant<
			    std::vector<common::TimedDataSequence<std::vector<std::vector<int8_t>>>>,
			    std::vector<common::TimedDataSequence<std::vector<std::vector<uint8_t>>>>,
			    std::vector<common::TimedDataSequence<std::vector<std::vector<int16_t>>>>,
			    std::vector<common::TimedDataSequence<std::vector<std::vector<uint16_t>>>>>
			    EntryPerSynapse;

			/**
			 * Entry per logical neuron, where for each sample the outer dimension of the data
			 * are the logical neurons of the population and the inner dimensions are the performed
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

			std::map<std::string, std::map<size_t, EntryPerSynapse>> data_per_synapse;
			std::map<std::string, std::map<size_t, EntryPerNeuron>> data_per_neuron;
			std::map<std::string, EntryArray> data_array;

			bool operator==(TimedData const& other) const = default;
			bool operator!=(TimedData const& other) const = default;

			GENPYBIND(stringstream)
			friend std::ostream& operator<<(std::ostream& os, TimedData const& value)
			    SYMBOL_VISIBLE;
		};

		/**
		 * Recorded data.
		 */
		typedef std::variant<RawData, TimedData> Data;

		Data data;

		Results(Data data);

		virtual size_t batch_size() const override;

		std::unique_ptr<PortData> copy() const override;
		std::unique_ptr<PortData> move() override;

	protected:
		virtual bool is_equal_to(PortData const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;
	};

	/**
	 * Results port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ResultsPortType
	    : public dapr::EmptyProperty<ResultsPortType, grenade::common::VertexPortType>
	{};

	typedef signal_flow::vertex::PlasticityRule::RawRecording RawRecording GENPYBIND(opaque(false));
	typedef signal_flow::vertex::PlasticityRule::ID ID GENPYBIND(opaque(false));
	typedef signal_flow::vertex::PlasticityRule::TimedRecording TimedRecordingConfig
	    GENPYBIND(opaque(false));
	typedef signal_flow::vertex::PlasticityRule::Recording RecordingConfig;

	PlasticityRule(
	    std::optional<RecordingConfig> const& recording,
	    ID const& id,
	    std::vector<std::reference_wrapper<grenade::common::MultiIndexSequence const>>
	        population_shapes,
	    std::vector<std::reference_wrapper<grenade::common::MultiIndexSequence const>>
	        projection_shapes,
	    grenade::common::TimeDomainOnTopology time_domain,
	    std::optional<grenade::common::ExecutionInstanceOnExecutor> const&
	        execution_instance_on_executor =
	            std::optional<grenade::common::ExecutionInstanceOnExecutor>{std::nullopt});

	std::optional<RecordingConfig> recording;

	ID id;

	/**
	 * Only plasticity rule parameterization (input port index size-2) and dynamics (input port
	 * index size-1) are valid.
	 */
	virtual bool valid_input_port_data(
	    size_t input_port_on_vertex, grenade::common::PortData const& dynamics) const;

	/**
	 * If the plasticity rule has a recording, only plasticity rule results are valid for output
	 * port index 0.
	 */
	virtual bool valid_output_port_data(
	    size_t output_port_on_vertex, grenade::common::PortData const& recording) const;

	/**
	 * Only ClockCycleTimeDomainRuntimes are valid.
	 */
	virtual bool valid(grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const;

	void set_population_shapes(
	    std::vector<std::reference_wrapper<grenade::common::MultiIndexSequence const>> value);
	void set_projection_shapes(
	    std::vector<std::reference_wrapper<grenade::common::MultiIndexSequence const>> value);
	std::vector<std::unique_ptr<grenade::common::MultiIndexSequence>> get_projection_shapes() const;

	/**
	 * Get input ports to the plasticity rule.
	 *
	 * For each projection shape there's an input port (which is to be connected to the projection's
	 * synapse parameterization port), followed by an input port for each population shape (which is
	 * to be connected to the population's analog observable port). The second to last input port is
	 * the parameterization, while the last input port is the dynamics.
	 */
	virtual std::vector<Port> get_input_ports() const;

	/**
	 * Get output ports from the plasticity rule.
	 *
	 * If the plasticity rule has a recording, it features an output port (port index 0), which
	 * exposes the recording data.
	 */
	virtual std::vector<Port> get_output_ports() const;

	virtual std::optional<grenade::common::TimeDomainOnTopology> get_time_domain() const;
	void set_time_domain(grenade::common::TimeDomainOnTopology const& value);

	std::unique_ptr<Vertex> copy() const;
	std::unique_ptr<Vertex> move();

protected:
	virtual std::ostream& print(std::ostream& os) const;
	virtual bool is_equal_to(Vertex const& other) const;

private:
	std::vector<dapr::PropertyHolder<grenade::common::MultiIndexSequence>> m_population_shapes;
	std::vector<dapr::PropertyHolder<grenade::common::MultiIndexSequence>> m_projection_shapes;
	grenade::common::TimeDomainOnTopology m_time_domain;
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

} // namespace abstract
} // namespace grenade::vx::network
