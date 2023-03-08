#pragma once
#include "grenade/vx/network/placed_logical/plasticity_rule.h"
#include "hate/visibility.h"
#include <set>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include <pybind11/stl.h>
#endif


namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

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
	 * Generate plasticity rule which only executes given recording.
	 * Timing and projection information is left default/empty.
	 */
	PlasticityRule generate() const SYMBOL_VISIBLE;

private:
	std::set<Observable> m_observables;
};

} // namespace grenade::vx::network::placed_logical
