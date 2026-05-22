#pragma once
#include "dapr/property.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/parameter_interval.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

// CompartmentConnection
struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) CompartmentConnection
    : public dapr::Property<CompartmentConnection>
{
	struct ParameterSpace
	{
		struct Parameterization
		{
			virtual ~Parameterization();
		};

		virtual ~ParameterSpace();

		virtual bool valid(Parameterization const& parameterization) = 0;
	};

	virtual ~CompartmentConnection();
	CompartmentConnection() = default;
};


} // namepsace abstract
} // namepsace grenade::vx::network
