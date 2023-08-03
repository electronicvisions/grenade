#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/cadc_recording.h"
#include "grenade/vx/network/madc_recording.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/pad_recording.h"
#include "grenade/vx/network/plasticity_rule.h"
#include "grenade/vx/network/plasticity_rule_on_network.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/population_on_network.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/projection_on_network.h"
#include "hate/visibility.h"
#include <memory>

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

class GENPYBIND(visible) NetworkBuilder
{
public:
	/**
	 * Add on-chip population.
	 * The population is expected to feature unique and unused neuron locations.
	 * @param population Population to add
	 */
	PopulationOnNetwork add(
	    Population const& population,
	    common::ExecutionInstanceID const& execution_instance = common::ExecutionInstanceID())
	    SYMBOL_VISIBLE;

	/**
	 * Add off-chip population.
	 * @param population Population to add
	 */
	PopulationOnNetwork add(
	    ExternalSourcePopulation const& population,
	    common::ExecutionInstanceID const& execution_instance = common::ExecutionInstanceID())
	    SYMBOL_VISIBLE;

	/**
	 * Add on-chip background spike source population.
	 * @param population Population to add
	 */
	PopulationOnNetwork add(
	    BackgroundSourcePopulation const& population,
	    common::ExecutionInstanceID const& execution_instance = common::ExecutionInstanceID())
	    SYMBOL_VISIBLE;

	/**
	 * Add projection between already added populations.
	 * The projection is expected to be free of single connections present in already added
	 * projections. A single connection is considered equal, if it connects the same pre- and
	 * post-synaptic neurons and features the same receptor type.
	 * @param projection Projection to add
	 */
	ProjectionOnNetwork add(
	    Projection const& projection,
	    common::ExecutionInstanceID const& execution_instance = common::ExecutionInstanceID())
	    SYMBOL_VISIBLE;

	/**
	 * Add MADC recording of a single neuron.
	 * Only one MADC recording per network is allowed.
	 * If another recording is present at the recorded neuron, their source specification is
	 * required to match.
	 * @param madc_recording MADC recording to add
	 */
	void add(
	    MADCRecording const& madc_recording,
	    common::ExecutionInstanceID const& execution_instance = common::ExecutionInstanceID())
	    SYMBOL_VISIBLE;

	/**
	 * Add CADC recording of a collection of neurons.
	 * Only one CADC recording per network is allowed.
	 * If another recording is present at a recorded neuron, their source specification is required
	 * to match.
	 * @param cadc_recording CADC recording to add
	 */
	void add(
	    CADCRecording const& cadc_recording,
	    common::ExecutionInstanceID const& execution_instance = common::ExecutionInstanceID())
	    SYMBOL_VISIBLE;

	/**
	 * Add pad recording of a collection of neurons.
	 * Only one pad recording per network is allowed.
	 * If another recording is present at a recorded neuron, their source specification is required
	 * to match.
	 * @param pad_recording pad recording to add
	 */
	void add(
	    PadRecording const& pad_recording,
	    common::ExecutionInstanceID const& execution_instance = common::ExecutionInstanceID())
	    SYMBOL_VISIBLE;

	/*
	 * Add plasticity rule on already added projections.
	 * @param plasticity_rule PlasticityRule to add
	 */
	PlasticityRuleOnNetwork add(
	    PlasticityRule const& plasticity_rule,
	    common::ExecutionInstanceID const& execution_instance = common::ExecutionInstanceID())
	    SYMBOL_VISIBLE;

	NetworkBuilder() SYMBOL_VISIBLE;

	std::shared_ptr<Network> done() SYMBOL_VISIBLE;

private:
	std::map<common::ExecutionInstanceID, Network::ExecutionInstance> m_execution_instances;
	std::chrono::microseconds m_duration;
	log4cxx::LoggerPtr m_logger;
};

} // namespace grenade::vx::network
