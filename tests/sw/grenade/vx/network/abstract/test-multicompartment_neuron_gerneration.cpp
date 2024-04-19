#include "grenade/vx/network/abstract/multicompartment_neuron_generator.h"
#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_ruleset.h"
#include "grenade/vx/test_helper/multicompartment_test_result.h"
#include "hate/timer.h"
#include <array>
#include <fstream>
#include <iostream>
#include <gtest/gtest.h>

// TO-DO Timer
using namespace grenade::vx::network::abstract;

TEST(multicompartment_neuron, RandomNeurons)
{
	const double num_test_runs = 10;

	const int compartments_num_max = 30;

	const bool filter_loops = true;


	const int synaptic_input_num_max = 1000;
	const bool synaptic_input_specific = false;


	// Array holds Test Results for Number of Compartments
	std::array<std::vector<TestResult>, compartments_num_max> results;

	NeuronGenerator neuron_generator;

	// Write Raw Data into CSV file
	// std::cout << "Writing to File" << std::endl;
	// std::ofstream file;
	// file.open("Test_Log_With_Loops.csv");

	for (int run_count = 0; run_count < num_test_runs; run_count++) {
		TestResult temp_result;

		for (int i = 1; i <= compartments_num_max; i++) {
			// Record time of one Test
			hate::Timer timer_test;
			// Loop over number of compartments
			std::cout << "MaxCompartments: " << i << std::endl;
			if (filter_loops) {
				int j = i - 1;
				std::cout << "MaxCompartmentConnections: " << j << std::endl;
				hate::Timer timer_generation;
				NeuronWithEnvironment generated = neuron_generator.generate(
				    i, j, synaptic_input_num_max, synaptic_input_specific, filter_loops);
				int time_generation = timer_generation.get_ns();
				temp_result.time_generation = time_generation;
				std::cout << "Number Compartments: " << generated.neuron.num_compartments()
				          << " Number CompartmentConnections: "
				          << generated.neuron.num_compartment_connections() << std::endl;

				CoordinateSystem coordinates;
				ResourceManager resources;
				resources.add_config(generated.neuron, generated.environment);
				PlacementAlgorithmRuleset algorithm;

				bool success = 0;
				bool fail = 0;
				hate::Timer timer_placement;
				try {
					std::cout << "Run" << std::endl;
					AlgorithmResult result =
					    algorithm.run(CoordinateSystem(), generated.neuron, resources);
					// Check if Result is valid
					if (algorithm.valid(result.coordinate_system, generated.neuron, resources)) {
						std::cout << "Success" << std::endl;
						temp_result.success = true;
						success = 1;
					}
				} catch (std::logic_error&) {
					std::cout << "Fail" << std::endl;
					temp_result.success = false;
					fail = 1;
				}
				int time_placement = timer_placement.get_ns();
				temp_result.time_placement = time_placement;
				if (!success && !fail) {
					throw std::invalid_argument("Error: No Fail and No Success: Result Invalid");
				}
			} else {
				for (int j = i - 1; j <= (i * (i - 1)) / 2; j++) {
					std::cout << "MaxCompartmentConnections: " << j << std::endl;
					hate::Timer timer_generation;
					NeuronWithEnvironment generated = neuron_generator.generate(
					    i, j, synaptic_input_num_max, synaptic_input_specific, filter_loops);
					int time_generation = timer_generation.get_ns();
					temp_result.time_generation = time_generation;
					std::cout << "Number Compartments: " << generated.neuron.num_compartments()
					          << " Number CompartmentConnections: "
					          << generated.neuron.num_compartment_connections() << std::endl;

					CoordinateSystem coordinates;
					ResourceManager resources;
					resources.add_config(generated.neuron, generated.environment);
					PlacementAlgorithmRuleset algorithm;

					bool success = 0;
					bool fail = 0;
					hate::Timer timer_placement;
					try {
						std::cout << "Run" << std::endl;
						AlgorithmResult result =
						    algorithm.run(CoordinateSystem(), generated.neuron, resources);
						// Check if Result is valid
						if (algorithm.valid(
						        result.coordinate_system, generated.neuron, resources)) {
							std::cout << "Success" << std::endl;
							temp_result.success = true;
							success = 1;
						}
					} catch (std::logic_error&) {
						std::cout << "Fail" << std::endl;
						temp_result.success = false;
						fail = 1;
					}
					int time_placement = timer_placement.get_ns();
					temp_result.time_placement = time_placement;
					if (!success && !fail) {
						throw std::invalid_argument(
						    "Error: No Fail and No Success: Result Invalid");
					}
				}
			}
			int time_total = timer_test.get_ns();
			temp_result.time_total = time_total;
			results[i - 1].push_back(temp_result);
			std::cout << results[i - 1].size() << "\n";

			// Write Results in File
			// file << i << ";" << temp_result.success << ";" << temp_result.time_total << ";"
			//     << temp_result.time_generation << ";" << temp_result.time_placement << ";"
			//     << temp_result.space_efficiency << "\n";
		}
	}


	std::cout << std::endl << "--------------------------------------------------" << std::endl;

	// Calculate Average and Print
	std::array<int, compartments_num_max> averaged_run_times_generation;
	std::array<int, compartments_num_max> averaged_run_times_placement;
	std::array<int, compartments_num_max> averaged_run_times_total;
	std::array<int, compartments_num_max> success_rates;
	std::array<int, compartments_num_max> space_efficiency;
	for (int i = 0; i < compartments_num_max; i++) {
		int total_run_time_generation = 0;
		int total_run_time_placement = 0;
		int total_run_time_total = 0;
		int total_success = 0;
		int total_space_efficiency = 0;
		for (auto test_result : results[i]) {
			total_run_time_generation += test_result.time_generation;
			total_run_time_placement += test_result.time_placement;
			total_run_time_total += test_result.time_total;
			total_space_efficiency += test_result.space_efficiency;
			if (test_result.success) {
				total_success++;
			}
		}
		averaged_run_times_generation[i] = total_run_time_generation / num_test_runs;
		averaged_run_times_placement[i] = total_run_time_placement / num_test_runs;
		averaged_run_times_total[i] = total_run_time_total / num_test_runs;
		success_rates[i] = total_success / num_test_runs;
		space_efficiency[i] = total_space_efficiency / num_test_runs;
		std::cout << "Result CompartmentNum=" << i + 1 << std::endl
		          << "Run Time Total: " << averaged_run_times_total[i] << std::endl
		          << "Run Time Generation: " << averaged_run_times_generation[i] << std::endl
		          << "Run Time Placement: " << averaged_run_times_placement[i] << std::endl
		          << "Success Rate: " << success_rates[i] << std::endl
		          << "Space Efficiency: " << space_efficiency[i] << std::endl;
	}
	std::cout << "--------------------------------------------------" << std::endl;

	// Write Raw Data into CSV file
	/*
	file.close();
	std::cout << "Writing to File" << std::endl;
	std::ofstream file_Final;
	file_Final.open("Test_Log_With_Loops_Final.csv");
	for (int i = 0; i < compartments_num_max; i++) {
	    for (auto test_result : results[i]) {
	        file_Final << i << ";" << test_result.success << ";" << test_result.time_total << ";"
	                   << test_result.time_generation << ";" << test_result.time_placement << ";"
	                   << test_result.space_efficiency << "\n";
	    }
	}
	file_Final.close();
	std::cout << "Writing to File finished" << std::endl;
	*/
}