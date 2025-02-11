#pragma once

#include "grenade/vx/network/abstract/multicompartment_environment.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_resource/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_resource_with_constraint.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism.h"
#include "grenade/vx/network/abstract/parameter_interval.h"
#include <cmath>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {


// Mechanism for Membrane Capacitance
struct GENPYBIND(visible) SYMBOL_VISIBLE MechanismCapacitance : public Mechanism
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
	struct ParameterSpace
	{
		// Interval with range of Parameters
		ParameterInterval<double> capacitance_interval;

		struct Parameterization
		{
			Parameterization() = default;
			Parameterization(double const& value);
			double capacitance;

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
		    ParameterInterval<double> const& parameter_interval_in,
		    Parameterization const& parameterization_in);
	};

	ParameterSpace parameter_space;

	// Constructor
	MechanismCapacitance() = default;
	MechanismCapacitance(ParameterSpace const& parameter_space_in);

protected:
	bool is_equal_to(Mechanism const& other) const;
	std::ostream& print(std::ostream& os) const;
};


} // namespace abstract
} // namespace grenade::vx::network
