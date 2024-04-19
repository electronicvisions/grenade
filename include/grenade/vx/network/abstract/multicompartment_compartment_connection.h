#pragma once
#include "grenade/common/property.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/parameter_interval.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

// CompartmentConnection
struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) CompartmentConnection
    : public common::Property<CompartmentConnection>
{
	CompartmentConnection() = default;
	struct ParameterSpace
	{
		struct Parameterization
		{};
		virtual bool valid(Parameterization const& parameterization) = 0;
	};
};


} // namepsace abstract
} // namepsace grenade::vx::network
