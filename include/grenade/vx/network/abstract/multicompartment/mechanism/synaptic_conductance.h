#pragma once

#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_constraints.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_excitatory.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_inhibitory.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_input.h"
#include "grenade/vx/network/abstract/parameter_interval.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

// Mechanism for Synaptic Conductance
struct GENPYBIND(visible) SYMBOL_VISIBLE MechanismSynapticInputConductance
    : public MechanismSynapticInput
{
	// Parameter Space
	struct GENPYBIND(visible) SYMBOL_VISIBLE ParameterSpace : public Mechanism::ParameterSpace
	{
		// Interval with range of Parameters
		std::vector<ParameterInterval<double>> conductance_interval;
		std::vector<ParameterInterval<double>> potential_interval;
		std::vector<ParameterInterval<double>> time_constant_interval;

		struct GENPYBIND(visible) SYMBOL_VISIBLE Parameterization
		    : public Mechanism::ParameterSpace::Parameterization
		{
			Parameterization() = default;
			Parameterization(
			    std::vector<double> conductance_in,
			    std::vector<double> potential_in,
			    std::vector<double> time_constant_in);
			std::vector<double> conductance;
			std::vector<double> potential;
			std::vector<double> time_constant;

			virtual size_t size() const override;

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
		    std::vector<ParameterInterval<double>> interval_conductance,
		    std::vector<ParameterInterval<double>> interval_potential,
		    std::vector<ParameterInterval<double>> interval_time_constant);

		// Property methods
		std::unique_ptr<Mechanism::ParameterSpace> copy() const;
		std::unique_ptr<Mechanism::ParameterSpace> move();
		bool is_equal_to(Mechanism::ParameterSpace const& other) const;
		std::ostream& print(std::ostream& os) const;

		virtual size_t size() const override;
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
	MechanismSynapticInputConductance() = default;

	/**
	 * Construct conductance-based synaptic input mechanism.
	 * @param receptor_type Receptor type to provide
	 * @param enable_analog_readout Whether to provide an output channel for analog readout
	 */
	MechanismSynapticInputConductance(
	    ReceptorType receptor_type, bool enable_analog_readout = false);

protected:
	bool is_equal_to(Mechanism const& other) const;
	std::ostream& print(std::ostream& os) const;
};


} // namespace abstract
} // namespace grenade::vx::network
