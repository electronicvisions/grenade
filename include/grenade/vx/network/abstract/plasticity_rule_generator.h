#pragma once
#include "grenade/vx/network/abstract/plasticity_rule.h"
#include "hate/visibility.h"
#include <set>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include <pybind11/stl.h>
#endif


namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Generator for plasticity rule, which only records given observables.
 */
struct GENPYBIND(visible) OnlyRecordingPlasticityRuleGenerator
{
	/**
	 * Observables, which can be recorded.
	 */
	enum class Observable
	{
		weights,
		correlation_causal,
		correlation_acausal
	};

	OnlyRecordingPlasticityRuleGenerator(std::set<Observable> const& observables) SYMBOL_VISIBLE;

	/**
	 * Generate plasticity rule recording and parameterization.
	 */
	std::pair<PlasticityRule::TimedRecordingConfig, PlasticityRule::Parameterization> generate()
	    const SYMBOL_VISIBLE;

private:
	std::set<Observable> m_observables;
};

} // namespace abstract
} // namespace grenade::vx::network
