#pragma once

#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource_with_constraint.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism.h"
#include "grenade/vx/network/abstract/parameter_interval.h"
#include <cmath>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {


// Mechanism for Membrane Capacitance
struct GENPYBIND(visible) SYMBOL_VISIBLE MechanismCapacitance : public Mechanism
{
	// Parameter Space
	struct GENPYBIND(visible) ParameterSpace : public Mechanism::ParameterSpace
	{
		// Interval with range of Parameters
		ParameterInterval<double> capacitance_interval;

		struct GENPYBIND(visible) Parameterization
		    : public Mechanism::ParameterSpace::Parameterization

		{
			Parameterization() = default;
			Parameterization(double const& value);
			double capacitance;

			// Operators
			bool operator==(Parameterization const& other) const = default;
			bool operator!=(Parameterization const& other) const = default;

			// Property methods
			std::unique_ptr<Mechanism::ParameterSpace::Parameterization> copy() const;
			std::unique_ptr<Mechanism::ParameterSpace::Parameterization> move();
			bool is_equal_to(Mechanism::ParameterSpace::Parameterization const& other) const;
			std::ostream& print(std::ostream& os) const;
		};

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
		ParameterSpace(ParameterInterval<double> const& parameter_interval_in);

		// Property methods
		std::unique_ptr<Mechanism::ParameterSpace> copy() const;
		std::unique_ptr<Mechanism::ParameterSpace> move();
		bool is_equal_to(Mechanism::ParameterSpace const& other) const;
		std::ostream& print(std::ostream& os) const;
	};

	// Check for Conflict with itself when placed on Compartment
	bool conflict(Mechanism const& other) const;

	/**
	 * Check if paramter space is valid for the mechanism.
	 * @param parameter_space Parameter-Space to check valditiy for.
	 */
	bool valid(Mechanism::ParameterSpace const& parameter_space) const;

	// Return HardwareRessource Requirements
	HardwareResourcesWithConstraints get_hardware(
	    CompartmentOnNeuron const& compartment,
	    Mechanism::ParameterSpace const& mechanism_parameter_space,
	    Environment const& environment) const;

	// Copy
	std::unique_ptr<Mechanism> copy() const;
	std::unique_ptr<Mechanism> move();

	// Constructor
	MechanismCapacitance() = default;

protected:
	bool is_equal_to(Mechanism const& other) const;
	std::ostream& print(std::ostream& os) const;
};


} // namespace abstract
} // namespace grenade::vx::network
