#pragma once
#include "dapr/property.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/parameter_interval.h"
#include "hate/visibility.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

// CompartmentConnection
struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) CompartmentConnection
    : public dapr::Property<CompartmentConnection>
{
	struct GENPYBIND(inline_base("*")) SYMBOL_VISIBLE ParameterSpace
	    : public dapr::Property<ParameterSpace>
	{
		struct GENPYBIND(inline_base("*")) SYMBOL_VISIBLE Parameterization
		    : public dapr::Property<Parameterization>
		{
			virtual size_t size() const = 0;

			virtual std::unique_ptr<Parameterization> get_section(
			    grenade::common::MultiIndexSequence const& sequence) const = 0;

			virtual ~Parameterization();
		};

		virtual ~ParameterSpace();

		virtual std::unique_ptr<ParameterSpace> get_section(
		    grenade::common::MultiIndexSequence const& sequence) const = 0;

		virtual size_t size() const = 0;
		virtual bool valid(Parameterization const& parameterization) const = 0;
	};

	virtual ~CompartmentConnection();
	CompartmentConnection() = default;
};


} // namepsace abstract
} // namepsace grenade::vx::network
