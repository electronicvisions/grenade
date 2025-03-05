#pragma once

#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_excitatory.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_inhibitory.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource_with_constraint.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism.h"
#include "grenade/vx/network/abstract/multicompartment/synaptic_input_environment/conductance.h"
#include "grenade/vx/network/abstract/parameter_interval.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {


// Mechanism for Synaptic Conductance
struct GENPYBIND(visible) SYMBOL_VISIBLE MechanismSynapticInputConductance : public Mechanism
{
	// Check for Conflict with itself when placed on Compartment
	bool conflict(Mechanism const& other) const;

	// Return HardwareRessource Requirements
	HardwareResourcesWithConstraints get_hardware(
	    CompartmentOnNeuron const& compartment, Environment const& environment) const;

	// Copy
	std::unique_ptr<Mechanism> copy() const;
	std::unique_ptr<Mechanism> move();

	// Validate
	bool valid() const;

	// Parameter Space
	struct GENPYBIND(visible) ParameterSpace
	{
		// Interval with range of Parameters
		ParameterInterval<double> conductance_interval;
		ParameterInterval<double> potential_interval;
		ParameterInterval<double> time_constant_interval;

		struct GENPYBIND(visible) Parameterization
		{
			Parameterization() = default;
			Parameterization(
			    double const& conductance_in,
			    double const& potential_in,
			    double const& time_constant_in);
			double conductance;
			double potential;
			double time_constant;

			// Operators
			bool operator==(Parameterization const& other) const = default;
			bool operator!=(Parameterization const& other) const = default;
		};

		Parameterization parameterization;

		// Check if Parameterization is within ParameterSpace
		bool valid() const;

		// Operators
		bool operator==(ParameterSpace const& other) const = default;
		bool operator!=(ParameterSpace const& other) const = default;

		// Constructor
		ParameterSpace() = default;
		ParameterSpace(
		    ParameterInterval<double> const& interval_conductance,
		    ParameterInterval<double> const& interval_potential,
		    ParameterInterval<double> const& interval_time_constant,
		    Parameterization const& paramterization_in);
	};

	ParameterSpace parameter_space;

	// Constructor
	MechanismSynapticInputConductance() = default;
	MechanismSynapticInputConductance(ParameterSpace const& parameter_space_in);


protected:
	bool is_equal_to(Mechanism const& other) const;
	std::ostream& print(std::ostream& os) const;

private:
	// Convert Number of Inputs to Number of SynapticCircuits
	int round(int i) const;
};


} // namespace abstract
} // namespace grenade::vx::network
