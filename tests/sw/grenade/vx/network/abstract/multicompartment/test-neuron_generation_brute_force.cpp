#include "grenade/vx/network/abstract/multicompartment/placement/algorithm_brute_force.h"
#include "grenade/vx/test_helper/multicompartment_common_test_function.h"

#include <string>

#include <log4cxx/logger.h>

#include <gtest/gtest.h>

using namespace grenade::vx::network::abstract;

TEST(MulticompartmentNeuron, RandomNeuronsBruteForce)
{
	log4cxx::LoggerPtr logger =
	    log4cxx::Logger::getLogger("grenade.test.multicompartment.brute_force");

	// Test parameters
	const size_t num_runs = 1;
	const size_t max_num_compartments = 3;
	const size_t max_num_synaptic_inputs = 250;
	const std::string file_name = "";
	const size_t timeout = 10; //[s]

	PlacementAlgorithmBruteForce algorithm(timeout);

	test_neuron_placement(
	    file_name, logger, num_runs, max_num_compartments, max_num_synaptic_inputs, algorithm);
}
