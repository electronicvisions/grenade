#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_ruleset.h"
#include "grenade/vx/test_helper/multicompartment_common_test_function.h"

#include <log4cxx/logger.h>

#include <gtest/gtest.h>

// TO-DO Timer
using namespace grenade::vx::network::abstract;

TEST(MulticompartmentNeuron, RandomNeuronsRuleset)
{
	log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("grenade.test.multicompartment.ruleset");

	// Test parameters
	const size_t num_runs = 100;
	const size_t max_num_compartments = 10;
	const size_t max_num_synaptic_inputs = 5000;
	const std::string file_name = "";

	PlacementAlgorithmRuleset algorithm;

	test_neuron_placement(
	    file_name, logger, num_runs, max_num_compartments, max_num_synaptic_inputs, algorithm);
}
