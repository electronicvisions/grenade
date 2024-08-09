#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/background_source_population.h"
#include "grenade/vx/network/cadc_recording.h"
#include "grenade/vx/network/external_source_population.h"
#include "grenade/vx/network/inter_execution_instance_projection.h"
#include "grenade/vx/network/inter_execution_instance_projection_on_network.h"
#include "grenade/vx/network/madc_recording.h"
#include "grenade/vx/network/pad_recording.h"
#include "grenade/vx/network/plasticity_rule.h"
#include "grenade/vx/network/plasticity_rule_on_execution_instance.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/population_on_execution_instance.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/projection_on_execution_instance.h"
#include "hate/visibility.h"
#include <chrono>
#include <iosfwd>
#include <map>
#include <memory>
#include <variant>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include <pybind11/chrono.h>
#endif

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Placed but not routed network consisting of populations and projections.
 */
struct GENPYBIND(visible, holder_type("std::shared_ptr<grenade::vx::network::Network>")) Network
{
	/**
	 * Part of network requiring synchronized execution and realtime communication.
	 */
	struct ExecutionInstance
	{
		std::map<
		    PopulationOnExecutionInstance,
		    std::variant<Population, ExternalSourcePopulation, BackgroundSourcePopulation>>
		    populations;
		std::map<ProjectionOnExecutionInstance, Projection> projections;
		std::optional<MADCRecording> madc_recording;
		std::optional<CADCRecording> cadc_recording;
		std::optional<PadRecording> pad_recording;
		std::map<PlasticityRuleOnExecutionInstance, PlasticityRule> plasticity_rules;

		bool operator==(ExecutionInstance const& other) const SYMBOL_VISIBLE;
		bool operator!=(ExecutionInstance const& other) const SYMBOL_VISIBLE;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(
		    std::ostream& os, ExecutionInstance const& execution_instance) SYMBOL_VISIBLE;
	};

	std::map<grenade::common::ExecutionInstanceID, ExecutionInstance> const execution_instances;

	std::map<InterExecutionInstanceProjectionOnNetwork, InterExecutionInstanceProjection> const
	    inter_execution_instance_projections;

	std::vector<grenade::common::ExecutionInstanceID> const
	    topologically_sorted_execution_instance_ids;

	/**
	 * Duration spent during construction of network.
	 * This value is not compared in operator{==,!=}.
	 */
	std::chrono::microseconds const construction_duration;

	bool operator==(Network const& other) const SYMBOL_VISIBLE;
	bool operator!=(Network const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, Network const& network) SYMBOL_VISIBLE;
};

} // namespace network
} // namespace grenade::vx
