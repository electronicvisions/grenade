#pragma once

#include "ccalix/types.h"
#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_constraints.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/event_output.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism.h"
#include "grenade/vx/network/abstract/parameter_interval.h"
#include "haldls/vx/v3/cadc.h"
#include <cmath>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {


// Mechanism for EventOutput
struct GENPYBIND(visible) SYMBOL_VISIBLE MechanismEventOutput : public Mechanism
{
	// Parameter Space
	struct GENPYBIND(visible) ParameterSpace : public Mechanism::ParameterSpace
	{
		struct GENPYBIND(visible) Parameterization
		    : public Mechanism::ParameterSpace::Parameterization
		{
			Parameterization() = default;

			/**
			 * Construct parameterization with size of population.
			 */
			Parameterization(size_t size);

			virtual std::unique_ptr<Mechanism::ParameterSpace::Parameterization> get_section(
			    grenade::common::MultiIndexSequence const& sequence) const;

			// Operators
			bool operator==(Parameterization const& other) const = default;
			bool operator!=(Parameterization const& other) const = default;

			// Property methods
			std::unique_ptr<Mechanism::ParameterSpace::Parameterization> copy() const;
			std::unique_ptr<Mechanism::ParameterSpace::Parameterization> move();
			virtual size_t size() const override;

		protected:
			bool is_equal_to(Mechanism::ParameterSpace::Parameterization const& other) const;
			std::ostream& print(std::ostream& os) const;

		private:
			size_t m_size;
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
		/**
		 * Construct parameter space with size of population.
		 */
		ParameterSpace(size_t size);

		// Property methods
		std::unique_ptr<Mechanism::ParameterSpace> copy() const;
		std::unique_ptr<Mechanism::ParameterSpace> move();
		std::vector<ParameterInterval<ccalix::TimeInS>> time_constant;
		std::vector<ParameterInterval<haldls::vx::v3::CADCSampleQuad::Value>> potential;

		virtual size_t size() const override;

	protected:
		bool is_equal_to(Mechanism::ParameterSpace const& other) const;
		std::ostream& print(std::ostream& os) const;

	private:
		size_t m_size;
	};

	// Check for Conflict with itself when placed on the same Compartment
	bool conflict(Mechanism const& other) const;

	/**
	 * Check if parameter space is valid for the mechanism.
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
	MechanismEventOutput() = default;

	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const;
	virtual std::vector<grenade::common::Vertex::Port> get_output_ports() const;

protected:
	bool is_equal_to(Mechanism const& other) const;
	std::ostream& print(std::ostream& os) const;
};


} // namespace abstract
} // namespace grenade::vx::network
