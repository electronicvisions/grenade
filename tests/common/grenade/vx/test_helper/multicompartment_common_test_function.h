#pragma once

#include "grenade/vx/network/abstract/multicompartment/neuron.h"
#include "grenade/vx/network/abstract/multicompartment/neuron_generator.h"
#include "grenade/vx/network/abstract/multicompartment/placement/algorithm.h"
#include "grenade/vx/test_helper/multicompartment_test_result.h"
#include "halco/hicann-dls/vx/neuron.h"
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
 * @param mean_synaptic_inputs Mean number of synaptic input per neuron.
 * 		For each neuron, the number of synaptic inputs is drawn from a uniform distribution
 * 		between 0 and mean_synaptic_inputs / num_compartments * 2.
 * @param algorithm Placement-Algorithm for testing.
 * @param parallel If true, excute runs in parallel.
 * @param num_warmup Number of neurons to geenrate before staarting the test. This can be
 * 		helpful to warmup the caches.
 * @param max_neuron_columns Define the found placement as unsuccessfull if it exceeds this
 * 		number of neuron columns.
 * @param check_morphology_match Whether to test if the morphology of the mapped neuron agrees
 * 		with the morphology of the target neuron. This only works for algorithms which
 * 		preserve the compartment identifiers between the mapped neuron and the target
 * 		neuron.
 */
inline auto test_neuron_placement = [](std::string file_name,
                                       log4cxx::LoggerPtr logger,
                                       size_t num_runs,
                                       size_t max_num_compartments,
                                       size_t mean_synaptic_inputs,
                                       std::unique_ptr<PlacementAlgorithm> algorithm,
                                       bool parallel = true,
                                       size_t num_warmup = 0,
                                       size_t max_neuron_columns =
                                           halco::hicann_dls::vx::NeuronColumnOnLogicalNeuron::size,
                                       bool check_morphology_match = false) {
	// File for test result output.
	std::ofstream file;
	if (file_name != "") {
		file.open(file_name);

		file << "Number of compartments; Success of Placement; Total testing time; Time for "
		        "generation of neuron; Time for placement of neuron\n";
	}

	auto test_neuron_placement_run =
	    [mean_synaptic_inputs, max_neuron_columns, check_morphology_match](
	        log4cxx::LoggerPtr logger, size_t num_runs, size_t num_compartments,
	        std::unique_ptr<PlacementAlgorithm> algorithm,
	        size_t num_warmup) -> std::vector<TestResult> {
		std::vector<TestResult> results;
		NeuronGenerator neuron_generator;

		size_t inputs_per_comp = 0;
		if (mean_synaptic_inputs > 0) {
			inputs_per_comp = 2 * ((mean_synaptic_inputs - 1) / num_compartments + 1);
		}

		for (size_t run_count = 0; run_count < num_runs + num_warmup; run_count++) {
			TestResult result;
			result.num_compartments = num_compartments;
			result.success = false;
			hate::Timer timer_test;

			// Generate neuron
			hate::Timer timer_generation;
			NeuronWithEnvironmentAndParameterSpace generated = neuron_generator.generate(
			    num_compartments, num_compartments - 1, inputs_per_comp, false, true);
			result.time_generation = timer_generation.get_us();

			// Placement object
			ResourceManager resources;
			resources.add_config(
			    generated.neuron, generated.parameter_space, generated.environment);

			// Placement execution
			hate::Timer timer_placement;

			CoordinateSystem coordinates;
			algorithm->reset();
			AlgorithmResult placement_result;

			try {
				placement_result = algorithm->run(coordinates, generated.neuron, resources);
				result.success = placement_result.finished;
			} catch (std::runtime_error&) {
				result.success = false;
			}

			// Only declare mappings as a success if it fits on the coordinate system
			if (placement_result.coordinate_system.get_extent() > max_neuron_columns) {
				result.success = false;
			}

			// Test if morphology of mapped neuron agrees with target neuron
			if (check_morphology_match && result.success) {
				auto mapped = placement_result.coordinate_system.construct_neuron();
				if (!generated.neuron.has_equal_morphology(std::get<Neuron>(mapped))) {
					LOG4CXX_ERROR(
					    logger, "Target neuron and mapped neuron do not match.\n"
					                << "Target: " << generated.neuron
					                << "Mapped: " << std::get<Neuron>(mapped));
					throw std::runtime_error("Morphology of the neuron mapped by the algorithm "
					                         "does not match the morphology of the target neuron.");
				}
			}

			result.time_placement = timer_placement.get_us();
			result.time_total = timer_test.get_us();

			// Only save results when warmup is over
			if (run_count >= num_warmup) {
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
		}
		return results;
	};


	std::vector<std::future<std::vector<TestResult>>> run_results;
	if (parallel) {
		LOG4CXX_WARN(
		    logger, "Running in parallel mode: timings are not reliable for performance analysis.");
	}

	for (size_t num_compartments = 1; num_compartments <= max_num_compartments;
	     num_compartments++) {
		LOG4CXX_DEBUG(
		    logger, "Number of compartments: " + std::to_string(num_compartments) + " / " +
		                std::to_string(max_num_compartments));

		run_results.push_back(std::async(
		    parallel ? std::launch::async : std::launch::deferred, test_neuron_placement_run,
		    logger, num_runs, num_compartments, algorithm->clone(), num_warmup));
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
