#pragma once

#include "grenade/vx/network/abstract/parameter_interval.h"
#include "grenade/vx/network/abstract/property.h"

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

// CompartmentConnection
struct SYMBOL_VISIBLE GENPYBIND(visible) CompartmentConnection
    : public Property<CompartmentConnection>
{
	CompartmentConnection() = default;
	struct ParameterSpace
	{
		struct Parameterization
		{};
		virtual bool valid(Parameterization const& parameterization) = 0;
	};
};


} // namepsace grenade::vx::network