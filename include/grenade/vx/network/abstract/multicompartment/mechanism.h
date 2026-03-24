#pragma once

#include "dapr/map.h"
#include "dapr/property.h"
#include "grenade/common/vertex.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_constraint.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_constraints.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_environment.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_on_compartment.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

struct Environment;
struct CompartmentOnNeuron;

// Mechanism Base-Class
struct GENPYBIND(inline_base("*")) SYMBOL_VISIBLE Mechanism : public dapr::Property<Mechanism>
{
	struct GENPYBIND(inline_base("*")) ParameterSpace : public Property<Mechanism::ParameterSpace>
	{
		struct GENPYBIND(inline_base("*")) Parameterization
		    : public Property<Mechanism::ParameterSpace::Parameterization>
		{
			virtual ~Parameterization();

			virtual std::unique_ptr<Parameterization> get_section(
			    grenade::common::MultiIndexSequence const& sequence) const = 0;

			virtual size_t size() const = 0;
		};

		virtual ~ParameterSpace();

		virtual std::unique_ptr<ParameterSpace> get_section(
		    grenade::common::MultiIndexSequence const& sequence) const = 0;

		virtual size_t size() const = 0;

		/**
		 * Check if the given parameterization is valid for the parameter space.
		 * @param paramterization Parameterization to check validity for.
		 */
		virtual bool valid(Parameterization const& parameterization) const = 0;
	};

	virtual ~Mechanism();

	/**
	 * Check if the given paramter space is valid for the mechanism.
	 * @param paramter_space Paramter space to check for validity.
	 */
	virtual bool valid(ParameterSpace const& parameter_space) const = 0;

	virtual bool conflict(Mechanism const& other) const = 0;
	virtual HardwareConstraints get_hardware(
	    Mechanism::ParameterSpace const& parameter_space,
	    MechanismEnvironment const* environment) const = 0;

	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const = 0;
	virtual std::vector<grenade::common::Vertex::Port> get_output_ports() const = 0;
};

} // namespace abstract
} // namespace grenade::vx::network
