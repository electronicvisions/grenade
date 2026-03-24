#pragma once

#include "ccalix/types.h"
#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_constraints.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/fire.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism.h"
#include "grenade/vx/network/abstract/parameter_interval.h"
#include "haldls/vx/v3/cadc.h"
#include <cmath>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {


// Mechanism for threshold, reset and refractory period
struct GENPYBIND(visible) SYMBOL_VISIBLE MechanismFire : public Mechanism
{
	// Parameter Space
	struct GENPYBIND(visible) ParameterSpace : public Mechanism::ParameterSpace
	{
		struct GENPYBIND(visible) Parameterization
		    : public Mechanism::ParameterSpace::Parameterization
		{
			Parameterization() = default;
			Parameterization(
			    std::vector<haldls::vx::v3::CADCSampleQuad::Value> threshold_potential,
			    std::vector<haldls::vx::v3::CADCSampleQuad::Value> reset_potential,
			    std::vector<ccalix::TimeInS> refractory_time,
			    std::vector<ccalix::TimeInS> holdoff_time);

			virtual std::unique_ptr<Mechanism::ParameterSpace::Parameterization> get_section(
			    grenade::common::MultiIndexSequence const& sequence) const;

			// Operators
			bool operator==(Parameterization const& other) const = default;
			bool operator!=(Parameterization const& other) const = default;

			// Property methods
			std::unique_ptr<Mechanism::ParameterSpace::Parameterization> copy() const;
			std::unique_ptr<Mechanism::ParameterSpace::Parameterization> move();
			std::vector<haldls::vx::v3::CADCSampleQuad::Value> threshold_potential;
			std::vector<haldls::vx::v3::CADCSampleQuad::Value> reset_potential;
			std::vector<ccalix::TimeInS> refractory_time;
			std::vector<ccalix::TimeInS> holdoff_time;

			virtual size_t size() const override;

		protected:
			bool is_equal_to(Mechanism::ParameterSpace::Parameterization const& other) const;
			std::ostream& print(std::ostream& os) const;
		};

		virtual std::unique_ptr<Mechanism::ParameterSpace> get_section(
		    grenade::common::MultiIndexSequence const& sequence) const;

		/**
		 * Check if parameterization is valid for the parameter space.
		 * @param parameterization Paramterization to check validity for.
		 */
		bool valid(Mechanism::ParameterSpace::Parameterization const& parameterization) const;

		// Operators
		bool operator==(ParameterSpace const& other) const = default;
		bool operator!=(ParameterSpace const& other) const = default;

		// Constructor
		ParameterSpace() = default;
		ParameterSpace(
		    std::vector<CADCInterval> threshold_potential,
		    std::vector<CADCInterval> reset_potential,
		    std::vector<TimeInterval> refractory_time,
		    std::vector<TimeInterval> holdoff_time);

		// Property methods
		std::unique_ptr<Mechanism::ParameterSpace> copy() const;
		std::unique_ptr<Mechanism::ParameterSpace> move();
		std::vector<CADCInterval> threshold_potential;
		std::vector<CADCInterval> reset_potential;
		std::vector<TimeInterval> refractory_time;
		std::vector<TimeInterval> holdoff_time;

		virtual size_t size() const override;

	protected:
		bool is_equal_to(Mechanism::ParameterSpace const& other) const;
		std::ostream& print(std::ostream& os) const;
	};

	// Check for Conflict with itself when placed on same Compartment
	bool conflict(Mechanism const& other) const;

	/**
	 * Check if parameter space is valid for the mechanism.
	 * @param parameter_space Parameter-Space to check valditiy for.
	 */
	bool valid(Mechanism::ParameterSpace const& parameter_space) const;

	// return HardwareRessource requirements
	HardwareConstraints get_hardware(
	    Mechanism::ParameterSpace const& mechanism_parameter_space,
	    MechanismEnvironment const* environment) const;

	// Copy
	std::unique_ptr<Mechanism> copy() const;
	std::unique_ptr<Mechanism> move();

	// Constructor
	MechanismFire() = default;

	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const;
	virtual std::vector<grenade::common::Vertex::Port> get_output_ports() const;

protected:
	bool is_equal_to(Mechanism const& other) const;
	std::ostream& print(std::ostream& os) const;
};


} // namespace abstract
} // namespace grenade::vx::network
