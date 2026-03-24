#pragma once

#include "ccalix/types.h"
#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_constraints.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_excitatory.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_inhibitory.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_input.h"
#include "grenade/vx/network/abstract/parameter_interval.h"
#include "lola/vx/v3/neuron.h"
#include <vector>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

// Mechanism for Membran Capacitance
struct GENPYBIND(visible) SYMBOL_VISIBLE MechanismSynapticInputCurrent
    : public MechanismSynapticInput
{
	// Parameter Space
	struct GENPYBIND(visible) ParameterSpace : public Mechanism::ParameterSpace
	{
		// Interval with range of Parameters
		std::vector<AnalogValueInterval> i_synin_gm;
		std::vector<AnalogValueInterval> synapse_dac_bias;
		std::vector<TimeInterval> time_constant;

		struct GENPYBIND(visible) Parameterization
		    : public Mechanism::ParameterSpace::Parameterization
		{
			Parameterization() = default;
			Parameterization(
			    std::vector<lola::vx::v3::AtomicNeuron::AnalogValue> i_synin_gm,
			    std::vector<lola::vx::v3::AtomicNeuron::AnalogValue> synapse_dac_bias,
			    std::vector<ccalix::TimeInS> time_constant);

			std::vector<lola::vx::v3::AtomicNeuron::AnalogValue> i_synin_gm;
			std::vector<lola::vx::v3::AtomicNeuron::AnalogValue> synapse_dac_bias;
			std::vector<ccalix::TimeInS> time_constant;

			virtual std::unique_ptr<Mechanism::ParameterSpace::Parameterization> get_section(
			    grenade::common::MultiIndexSequence const& sequence) const;

			// Operators
			bool operator==(Parameterization const& other) const = default;
			bool operator!=(Parameterization const& other) const = default;

			// Property methods
			std::unique_ptr<Mechanism::ParameterSpace::Parameterization> copy() const;
			std::unique_ptr<Mechanism::ParameterSpace::Parameterization> move();
			bool is_equal_to(Mechanism::ParameterSpace::Parameterization const& other) const;
			std::ostream& print(std::ostream& os) const;

			virtual size_t size() const override;
		};

		virtual std::unique_ptr<Mechanism::ParameterSpace> get_section(
		    grenade::common::MultiIndexSequence const& sequence) const;

		/**
		 * Check if parameterization is valid for the paramter space.
		 * @param parameterization Paramterization to check validity for.
		 */
		bool valid(Mechanism::ParameterSpace::Parameterization const& parameterization) const;

		// Operators
		bool operator==(ParameterSpace const& other) const = default;
		bool operator!=(ParameterSpace const& other) const = default;

		// Constructor
		ParameterSpace() = default;
		ParameterSpace(
		    std::vector<AnalogValueInterval> i_synin_gm,
		    std::vector<AnalogValueInterval> synapse_dac_bias,
		    std::vector<TimeInterval> time_constant);

		virtual size_t size() const override;

		// Property methods
		std::unique_ptr<Mechanism::ParameterSpace> copy() const;
		std::unique_ptr<Mechanism::ParameterSpace> move();
		bool is_equal_to(Mechanism::ParameterSpace const& other) const;
		std::ostream& print(std::ostream& os) const;
	};

	/**
	 * Check if paramter space is valid for the mechanism.
	 * @param parameter_space Parameter-Space to check valditiy for.
	 */
	bool valid(Mechanism::ParameterSpace const& parameter_space) const;

	// Copy
	std::unique_ptr<Mechanism> copy() const;
	std::unique_ptr<Mechanism> move();

	// Constructor
	MechanismSynapticInputCurrent() = default;

	/**
	 * Construct current-based synaptic input mechanism.
	 * @param receptor_type Receptor type to provide
	 * @param enable_analog_readout Whether to provide an output channel for analog readout
	 */
	MechanismSynapticInputCurrent(ReceptorType receptor_type, bool enable_analog_readout = false);

protected:
	bool is_equal_to(Mechanism const& other) const;
	std::ostream& print(std::ostream& os) const;
};

} // namespace abstract
} // namespace grenade::vx::network
