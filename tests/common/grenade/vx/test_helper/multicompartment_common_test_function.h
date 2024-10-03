#pragma once

#include "grenade/vx/network/abstract/multicompartment_neuron_generator.h"
#include "grenade/vx/network/abstract/multicompartment_placement_algorithm.h"
#include "grenade/vx/test_helper/multicompartment_test_result.h"
#include "hate/timer.h"
#include <fstream>
#include <future>
#include <iostream>

#include <log4cxx/logger.h>

using namespace grenade::vx::network::abstract;

/**
 * Test function for different multicompartment-neuron placement algorithms.
 * Neurons with one to max_num_compartments are randomly generated and placed by the given
 * algorithm.
 * @param file_name Path of a file to write the test results to. If an empty
       string is supplied, the results are not written.
 * @param logger Logger for the test.
 * @param num_runs Number of test runs for each neuron size.
 * @param max_num_compartments Upper limit for compartments on a test neuron.
 * @param max_num_synaptic_inputs Upper limit for the number of synaptic inputs on a compartment.
 * @param algorithm Placement-Algorithm for testing.
 */
inline auto test_neuron_placement = [](std::string file_name,
                                       log4cxx::LoggerPtr logger,
                                       size_t num_runs,
                                       size_t max_num_compartments,
                                       size_t max_num_synaptic_inputs,
                                       PlacementAlgorithm& algorithm) {
	NeuronGenerator neuron_generator;

	// File for test result output.
	std::ofstream file;
	if (file_name != "") {
		file.open(file_name);

		file << "Number of compartments ; Success of Placement ; Total testing time ; Time for "
		        "generation of neuron ; Time for placement of neuron ; Space efficiency of "
		        "placement\n";
	}

	auto test_neuron_placement_run =
	    [&neuron_generator](
	        log4cxx::LoggerPtr logger, size_t max_num_compartments, size_t max_num_synaptic_inputs,
	        std::unique_ptr<PlacementAlgorithm> algorithm) -> std::vector<TestResult> {
		std::vector<TestResult> results;

		for (size_t num_compartments = 1; num_compartments <= max_num_compartments;
		     num_compartments++) {
			TestResult result;
			result.num_compartments = num_compartments;
			result.success = false;
			hate::Timer timer_test;

			// Generate neuron
			hate::Timer timer_generation;
			NeuronWithEnvironment generated = neuron_generator.generate(
			    num_compartments, num_compartments - 1, max_num_synaptic_inputs, false, true);
			result.time_generation = timer_generation.get_us();

			// Placement object
			ResourceManager resources;
			resources.add_config(generated.neuron, generated.environment);

			// Placement execution
			hate::Timer timer_placement;

			CoordinateSystem coordinates;
			algorithm->reset();
			AlgorithmResult placement_result;

			try {
				placement_result = algorithm->run(coordinates, generated.neuron, resources);
				result.success = placement_result.finished;
			} catch (std::logic_error&) {
				result.success = false;
			}

			result.time_placement = timer_placement.get_us();
			result.time_total = timer_test.get_us();
			results.push_back(result);

			if (result.success) {
				LOG4CXX_DEBUG(
				    logger, "Number of compartments: " + std::to_string(num_compartments) +
				                "Placement Successfull.");
			} else {
				LOG4CXX_DEBUG(
				    logger, "Number of compartments: " + std::to_string(num_compartments) +
				                "Placement Failed.");
			}
		}
		return results;
	};


	std::vector<std::future<std::vector<TestResult>>> run_results;

	for (size_t run_count = 0; run_count < num_runs; run_count++) {
		LOG4CXX_DEBUG(
		    logger, "Run: " + std::to_string(run_count) + " / " + std::to_string(num_runs));

		run_results.push_back(std::async(
		    std::launch::async, test_neuron_placement_run, logger, max_num_compartments,
		    max_num_synaptic_inputs, algorithm.clone()));
	}
	if (file_name != "") {
		for (auto& future : run_results) {
			std::vector<TestResult> results = future.get();
			for (auto result : results) {
				file << result.num_compartments << ";" << result.success << ";" << result.time_total
				     << ";" << result.time_generation << ";" << result.time_placement << "\n";
			}
		}

		file.close();
	}
};
