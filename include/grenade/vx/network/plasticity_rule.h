#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/projection.h"
#include "halco/common/geometry.h"
#include "hate/visibility.h"
#include <string>
#include <vector>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

/**
 * Plasticity rule.
 */
struct GENPYBIND(visible) PlasticityRule
{
	/**
	 * Descriptor to projections this rule has access to.
	 * All projections are required to be dense and in order.
	 */
	std::vector<ProjectionDescriptor> projections{};

	/**
	 * Plasticity rule kernel to be compiled into the PPU program.
	 */
	std::string kernel{};

	/**
	 * Timing information for execution of the rule.
	 */
	struct Timer
	{
		/** PPU clock cycles. */
		struct GENPYBIND(inline_base("*")) Value
		    : public halco::common::detail::RantWrapper<Value, uintmax_t, 0xffffffff, 0>
		{
			constexpr explicit Value(uintmax_t const value = 0) : rant_t(value) {}
		};

		Value start;
		Value period;
		size_t num_periods;

		bool operator==(Timer const& other) const SYMBOL_VISIBLE;
		bool operator!=(Timer const& other) const SYMBOL_VISIBLE;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Timer const& timer) SYMBOL_VISIBLE;
	} timer;

	PlasticityRule() = default;

	bool operator==(PlasticityRule const& other) const SYMBOL_VISIBLE;
	bool operator!=(PlasticityRule const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, PlasticityRule const& plasticity_rule)
	    SYMBOL_VISIBLE;
};


/** Descriptor to be used to identify a plasticity rule. */
struct GENPYBIND(inline_base("*")) PlasticityRuleDescriptor
    : public halco::common::detail::BaseType<PlasticityRuleDescriptor, size_t>
{
	constexpr explicit PlasticityRuleDescriptor(value_type const value = 0) : base_t(value) {}
};

} // namespace network

} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::PlasticityRuleDescriptor)

} // namespace std
