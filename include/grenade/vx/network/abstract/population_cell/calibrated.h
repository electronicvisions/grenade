#pragma once
#include "ccalix/types.h"
#include "dapr/empty_property.h"
#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/population.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "haldls/vx/v3/cadc.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <optional>
#include <variant>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Locally-placed neuron with calibrated parameterization.
 * The parameterization contains the to-be-calibrated model parameters per atomic neuron circuit.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) CalibratedNeuron : public LocallyPlacedNeuron
{
	struct ParameterSpace : public grenade::common::Population::Cell::ParameterSpace
	{
		struct CalibrationTarget
		{
			/**
			 * Selected membrane capacitance.
			 * The available range is 0 to approximately 2.2 pF, represented as 0 to 63 LSB.
			 */
			lola::vx::v3::AtomicNeuron::MembraneCapacitance::CapacitorSize membrane_capacitance{63};

			/**
			 * Membrane capacitance used during calibration.
			 * Might be set to be different than `membrane_capacitance` for use in
			 * larger-than-single-circuit compartments. The available range is 0 to
			 * approximately 2.2 pF, represented as 0 to 63 LSB. If empty, `membrane_capacitance` is
			 * used also during calibration.
			 */
			std::optional<lola::vx::v3::AtomicNeuron::MembraneCapacitance::CapacitorSize>
			    membrane_capacitance_during_calibration{std::nullopt};

			/**
			 * Target CADC read at leak (resting) potential.
			 */
			haldls::vx::v3::CADCSampleQuad::Value v_leak{80};

			/**
			 * Membrane time constant, disabled, if empty.
			 */
			std::optional<ccalix::TimeInS> tau_membrane{10e-6};

			/**
			 * Target CADC read near spike threshold.
			 * If empty, threshold mechanism is disabled.
			 */
			std::optional<haldls::vx::v3::CADCSampleQuad::Value> v_threshold{125};

			/**
			 * Target CADC read at reset potential.
			 */
			haldls::vx::v3::CADCSampleQuad::Value v_reset{70};

			/**
			 * Refractory period mechanism.
			 */
			struct RefractoryPeriod
			{
				/**
				 * Refractory time in us.
				 */
				ccalix::TimeInS refractory_time{2e-6};

				/**
				 * Target length of the holdoff period. The holdoff period is the time at the end of
				 * the refractory period in which the clamping to the reset voltage is already
				 * released but new spikes can still not be generated.
				 */
				ccalix::TimeInS holdoff_time{0e-6};

				RefractoryPeriod() = default;

				bool operator==(RefractoryPeriod const& other) const = default;
				bool operator!=(RefractoryPeriod const& other) const = default;

				GENPYBIND(stringstream)
				friend std::ostream& operator<<(std::ostream& os, RefractoryPeriod const& value)
				    SYMBOL_VISIBLE;
			};

			/**
			 * Refractory period mechanism.
			 * Disabled if empty.
			 */
			std::optional<RefractoryPeriod> refractory_period;

			struct CubaSynapticInput
			{
				/**
				 * Synaptic input time constant.
				 */
				ccalix::TimeInS tau_syn{10e-6};

				/**
				 * Synaptic input strength as CapMem bias current. All synaptic inputs of equal type
				 * are calibrated to a target measured as the median of all neurons at this setting.
				 */
				lola::vx::v3::AtomicNeuron::AnalogValue i_synin_gm{500};

				/**
				 * Synapse DAC bias current that is desired.
				 * Can be lowered in order to reduce the amplitude of a spike at the input of the
				 * synaptic input OTA. This can be useful to avoid saturation when using larger
				 * synaptic time constants.
				 * Has to be homogeneous per chip (quadrant, TODO: check whether implemented).
				 */
				lola::vx::v3::AtomicNeuron::AnalogValue synapse_dac_bias{600};

				CubaSynapticInput() = default;

				bool operator==(CubaSynapticInput const& other) const = default;
				bool operator!=(CubaSynapticInput const& other) const = default;

				GENPYBIND(stringstream)
				friend std::ostream& operator<<(std::ostream& os, CubaSynapticInput const& value)
				    SYMBOL_VISIBLE;
			};

			struct CobaSynapticInput
			{
				/**
				 * Synaptic input time constant.
				 */
				ccalix::TimeInS tau_syn{10e-6};

				/**
				 * COBA synaptic input reversal potential.
				 *
				 * At this potential, the synaptic input strength will be zero. The distance
				 * between COBA reversal and reference potential determines the strength of the
				 * amplitude modulation. Note that in biological context, the difference between
				 * reference and reversal potentials is a scaling factor for the conductance
				 * achieved by an input event. Given in CADC units. The values may exceed the
				 * dynamic range of leak and CADC. In this case, the calibration is performed at
				 * a lower, linearly interpolated value.
				 */
				double e_reversal{300};

				/**
				 * COBA synaptic input reference potential.
				 *
				 * At this potential, the original CUBA synaptic input strength, given via
				 * i_synin_gm, is not modified by COBA modulation. Optional: If None, the
				 * midpoint between leak and threshold will be used. Given in CADC units. The
				 * values must be reachable by the leak term, and the dynamic range of the CADC
				 * must allow for measurement of synaptic input amplitudes on top of this
				 * potential. We recommend choosing a value between the leak and threshold.
				 */
				std::optional<double> e_reference{std::nullopt};

				/**
				 * Synaptic input strength as CapMem bias current. All synaptic inputs of equal type
				 * are calibrated to a target measured as the median of all neurons at this setting.
				 */
				lola::vx::v3::AtomicNeuron::AnalogValue i_synin_gm{500};

				/**
				 * Synapse DAC bias current that is desired.
				 * Can be lowered in order to reduce the amplitude of a spike at the input of the
				 * synaptic input OTA. This can be useful to avoid saturation when using larger
				 * synaptic time constants.
				 * Has to be homogeneous per chip (quadrant, TODO: check whether implemented).
				 */
				lola::vx::v3::AtomicNeuron::AnalogValue synapse_dac_bias{600};

				CobaSynapticInput() = default;

				bool operator==(CobaSynapticInput const& other) const = default;
				bool operator!=(CobaSynapticInput const& other) const = default;

				GENPYBIND(stringstream)
				friend std::ostream& operator<<(std::ostream& os, CobaSynapticInput const& value)
				    SYMBOL_VISIBLE;
			};

			struct DisabledSynapticInput
			{
				DisabledSynapticInput() = default;

				bool operator==(DisabledSynapticInput const& other) const = default;
				bool operator!=(DisabledSynapticInput const& other) const = default;

				GENPYBIND(stringstream)
				friend std::ostream& operator<<(
				    std::ostream& os, DisabledSynapticInput const& value) SYMBOL_VISIBLE;
			};

			typedef std::variant<DisabledSynapticInput, CubaSynapticInput, CobaSynapticInput>
			    SynapticInput;

			/**
			 * Excitatory synaptic input.
			 */
			SynapticInput synaptic_input_excitatory;

			/**
			 * Inhibitory synaptic input.
			 */
			SynapticInput synaptic_input_inhibitory;

			/**
			 * Configuration and calibration of inter-atomic neuron connectivity.
			 */
			struct InterAtomicNeuronConnectivity
			{
				/** Connect local membrane to soma (short-circuit connection). */
				bool connect_soma;

				/** Connect soma to soma on the right (short-circuit connection). */
				bool connect_soma_right;

				/** Connect local membrane to membrane on the right (short-circuit connection). */
				bool connect_right;

				/** Connect local membrane vertically to membrane on the opposite hemisphere
				 * (short-circuit connection). */
				bool connect_vertical;

				/**
				 * Connect local membrane to soma via inter-compartment-conductance time constant.
				 * If empty, inter-compartment conductance is disabled.
				 */
				std::optional<ccalix::TimeInS> tau_icc;

				InterAtomicNeuronConnectivity() = default;

				bool operator==(InterAtomicNeuronConnectivity const& other) const = default;
				bool operator!=(InterAtomicNeuronConnectivity const& other) const = default;

				GENPYBIND(stringstream)
				friend std::ostream& operator<<(
				    std::ostream& os, InterAtomicNeuronConnectivity const& value) SYMBOL_VISIBLE;
			};

			InterAtomicNeuronConnectivity inter_atomic_neuron_connectivity;

			CalibrationTarget() = default;

			bool operator==(CalibrationTarget const& other) const = default;
			bool operator!=(CalibrationTarget const& other) const = default;

			GENPYBIND(stringstream)
			friend std::ostream& operator<<(std::ostream& os, CalibrationTarget const& value)
			    SYMBOL_VISIBLE;
		};

		/**
		 * Calibration targets per neuron on population per compartment on neuron per atomic neuron
		 * on compartment.
		 */
		typedef std::vector<
		    std::map<grenade::common::CompartmentOnNeuron, std::vector<CalibrationTarget>>>
		    CalibrationTargets;

		struct Parameterization
		    : public grenade::common::Population::Cell::ParameterSpace::Parameterization
		{
			/**
			 * Parameterization containing the same calibration targets as the parameter space until
			 * parameter transformation calibration is available.
			 */
			CalibrationTargets calibration_targets;

			/**
			 * Readout source to select per neuron on population per compartment on neuron per
			 * atomic neuron on compartment.
			 */
			typedef std::vector<std::map<
			    grenade::common::CompartmentOnNeuron,
			    std::vector<lola::vx::v3::AtomicNeuron::Readout::Source>>>
			    ReadoutSources;
			ReadoutSources readout_sources;

			Parameterization(
			    CalibrationTargets calibration_targets, ReadoutSources readout_sources);

			virtual size_t size() const override;

			virtual std::unique_ptr<grenade::common::PortData> copy() const override;
			virtual std::unique_ptr<grenade::common::PortData> move() override;

			virtual std::unique_ptr<
			    grenade::common::Population::Cell::ParameterSpace::Parameterization>
			get_section(grenade::common::MultiIndexSequence const& sequence) const override;

		protected:
			virtual bool is_equal_to(grenade::common::PortData const& other) const override;
			virtual std::ostream& print(std::ostream& os) const override;
		};

		CalibrationTargets calibration_targets;

		ParameterSpace(CalibrationTargets calibration_targets);

		virtual size_t size() const override;

		virtual bool valid(
		    size_t input_port_on_cell,
		    grenade::common::Population::Cell::ParameterSpace::Parameterization const&
		        parameterization) const override;

		virtual std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> copy()
		    const override;
		virtual std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> move() override;

		virtual std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> get_section(
		    grenade::common::MultiIndexSequence const& sequence) const override;

	protected:
		virtual bool is_equal_to(
		    grenade::common::Population::Cell::ParameterSpace const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;
	};

	CalibratedNeuron(
	    Compartments compartments, halco::hicann_dls::vx::v3::LogicalNeuronCompartments shape);

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	/**
	 * Only CalibratedNeuron parameter space is valid.
	 */
	virtual bool valid(
	    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const override;

	/**
	 * Get input ports.
	 * The CalibratedNeuron features the same input ports as the LocallyPlacedNeuron.
	 * Additionally, it features a port for parameterization (port index 1).
	 */
	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const override;

	virtual std::unique_ptr<grenade::common::Population::Cell> copy() const override;
	virtual std::unique_ptr<grenade::common::Population::Cell> move() override;

protected:
	virtual bool is_equal_to(grenade::common::Population::Cell const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
