#include "grenade/vx/network/abstract/multicompartment/placement/algorithm_evolutionary.h"
#include "grenade/vx/test_helper/multicompartment_common_test_function.h"

#include <future>
#include <vector>

#include <log4cxx/logger.h>

#include <gtest/gtest.h>

using namespace grenade::vx::network::abstract;

TEST(MulticompartmentNeuron, RandomNeuronsEvolutionary)
{
	log4cxx::LoggerPtr logger =
	    log4cxx::Logger::getLogger("grenade.test.multicompartment.evolutionary");

	// Test parameters
	const size_t num_runs = 1;
	const size_t max_num_compartments = 3;
	const size_t max_num_synaptic_inputs = 1000;
	const std::string file_name = "";
	const size_t timeout = 10;


	// Create Parameter-Configurations
	std::vector<double> p_mutate_individual = {0.4};
	std::vector<double> p_mutate_gene = {0.01};
	std::vector<double> p_mutate_initial = {0.8};
	std::vector<double> p_shift = {0.1};
	std::vector<double> p_add = {0.1};
	std::vector<double> p_remove = {0.1};
	std::vector<double> p_mate = {0.2};
	std::vector<double> x_max = {10};
	std::vector<size_t> seeds = {0};
	std::vector<size_t> pop_size = {1000};

	std::vector<EvolutionaryParameters> configuration_run_parameters;

	for (auto population_size : pop_size) {
		for (auto seed : seeds) {
			for (auto x : x_max) {
				for (auto mutate_individual : p_mutate_individual) {
					for (auto mutate_gene : p_mutate_gene) {
						for (auto mutate_initial : p_mutate_initial) {
							for (auto shift : p_shift) {
								for (auto add : p_add) {
									for (auto remove : p_remove) {
										for (auto mate : p_mate) {
											auto run_parameters = EvolutionaryParameters();
											run_parameters.p_mutate_individual = mutate_individual;
											run_parameters.p_mutate_gene = mutate_gene;
											run_parameters.p_mutate_initial = mutate_initial;
											run_parameters.p_shift = shift;
											run_parameters.p_add = add;
											run_parameters.p_remove = remove;
											run_parameters.p_mate = mate;
											run_parameters.seed = seed;
											run_parameters.x_max = x;
											run_parameters.population_size = population_size;

											run_parameters.time_limit = timeout;
											run_parameters.run_limit = 1;
											run_parameters.tournament_contestants = 2;
											run_parameters.mating_block_size =
											    3 * run_parameters.x_max;
											run_parameters.min_shift = -4;
											run_parameters.max_shift = 4;
											run_parameters.together_add_remove = false;
											run_parameters.number_columns_add_remove = 1;
											run_parameters.lower_limit_add_remove = 1;
											run_parameters.upper_limit_add_remove =
											    run_parameters.x_max - 1;
											run_parameters.number_hall_of_fame =
											    0.05 * run_parameters.population_size;

											run_parameters.parallel_threads = 50;

											configuration_run_parameters.push_back(run_parameters);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}


	std::vector<std::future<void>> void_results;

	// Execute test for each different parameterization
	for (size_t i = 0; i < configuration_run_parameters.size(); i++) {
		std::string file_name_configuration = std::to_string(i) + file_name;

		EvolutionaryParameters& run_parameter = configuration_run_parameters.at(i);

		// Write header to csv file
		std::ofstream file;
		if (file_name != "") {
			file.open(file_name_configuration);
			file << "Run parameters:"
			     << "\nProbability to mutate individual:   " << run_parameter.p_mutate_individual
			     << "\nProbability to mutate single gene:  " << run_parameter.p_mutate_gene
			     << "\nProbability for initial mutation:   " << run_parameter.p_mutate_initial
			     << "\nProbability for shifting:           " << run_parameter.p_shift
			     << "\nProbability for adding columns:     " << run_parameter.p_add
			     << "\nProbability for removing columns:   " << run_parameter.p_remove
			     << "\nProbability for mating:             " << run_parameter.p_mate
			     << "\nUpper limit of coordinate system:   " << run_parameter.x_max
			     << "\nSeed for random number generator:   " << run_parameter.seed << "\n\n";
			file.close();
		} else {
			file_name_configuration = "";
		};

		PlacementAlgorithmEvolutionary algorithm(run_parameter);

		void_results.push_back(std::async(
		    std::launch::async, test_neuron_placement, file_name_configuration, logger, num_runs,
		    max_num_compartments, max_num_synaptic_inputs, std::ref(algorithm)));
	}


	for (auto& void_result : void_results) {
		void_result.wait();
	}

	LOG4CXX_INFO(
	    logger,
	    "Finished multicompartment placement test with evolutionary algorithm succesfully.");
}