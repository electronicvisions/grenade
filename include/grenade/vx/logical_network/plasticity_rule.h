#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/logical_network/projection.h"
#include "grenade/vx/network/plasticity_rule.h"
#include "halco/common/geometry.h"
#include "hate/visibility.h"
#include <string>
#include <vector>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace logical_network GENPYBIND_MODULE {

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
	typedef network::PlasticityRule::Timer Timer;
	Timer timer;

	/**
	 * Enable whether this plasticity rule requires all projections to have one source per row and
	 * them being in order.
	 */
	bool enable_requires_one_source_per_row_in_order{false};

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

} // namespace logical_network

GENPYBIND_MANUAL({
	parent.attr("logical_network").attr("PlasticityRule").attr("Timer") =
	    parent.attr("PlasticityRule").attr("Timer");
})

} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::logical_network::PlasticityRuleDescriptor)

} // namespace std
