#pragma once

#include "ccalix/types.h"
#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_constraints.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/with_analog_readout.h"
#include "grenade/vx/network/abstract/parameter_interval.h"
#include <cmath>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {


// Mechanism for Membrane Capacitance
struct GENPYBIND(visible) SYMBOL_VISIBLE MechanismCapacitance : public MechanismWithAnalogReadout
{
	// Parameter Space
	struct GENPYBIND(visible) ParameterSpace : public Mechanism::ParameterSpace
	{
		// Interval with range of Parameters
		std::vector<CapacitanceInterval> capacitance;

		virtual size_t size() const override;

		struct GENPYBIND(visible) Parameterization
		    : public Mechanism::ParameterSpace::Parameterization
		{
			Parameterization() = default;
			Parameterization(std::vector<ccalix::CapacitanceInFarad> value);
			std::vector<ccalix::CapacitanceInFarad> capacitance;

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
		ParameterSpace(std::vector<CapacitanceInterval> parameter_interval_in);

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
	HardwareConstraints get_hardware(
	    Mechanism::ParameterSpace const& mechanism_parameter_space,
	    MechanismEnvironment const* environment) const;

	// Copy
	std::unique_ptr<Mechanism> copy() const;
	std::unique_ptr<Mechanism> move();

	// Constructor
	MechanismCapacitance(bool enable_analog_readout = false);

	virtual lola::vx::v3::AtomicNeuron::Readout::Source get_analog_readout_source() const override;

protected:
	bool is_equal_to(Mechanism const& other) const;
	std::ostream& print(std::ostream& os) const;
};


} // namespace abstract
} // namespace grenade::vx::network
