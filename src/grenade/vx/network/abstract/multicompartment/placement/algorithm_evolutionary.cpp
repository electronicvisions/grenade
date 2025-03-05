#include "grenade/vx/network/abstract/multicompartment/placement/algorithm_evolutionary.h"
#include <log4cxx/logger.h>

namespace grenade::vx::network::abstract {

PlacementAlgorithmEvolutionary::PlacementAlgorithmEvolutionary() :
    m_logger(log4cxx::Logger::getLogger("grenade.MC.PlacementAlgorithmEvolutionary"))
{
	// Run
	m_run_parameters.seed = 0;
	m_run_parameters.time_limit = 1 * 60; // s
	m_run_parameters.run_limit = 1;
	m_run_parameters.x_max = 20;
	m_run_parameters.parallel_threads = 50;

	// Population
	m_run_parameters.population_size = 10000;

	// Selection
	m_run_parameters.number_hall_of_fame = 10; // 10;
	m_run_parameters.tournament_contestants = 2;

	// Mating
	m_run_parameters.p_mate = 0.2;
	m_run_parameters.mating_block_size = 3 * m_run_parameters.x_max;

	// Random mutation
	m_run_parameters.p_mutate_individual = 0.4;
	m_run_parameters.p_mutate_gene = 0.01;
	m_run_parameters.p_mutate_initial = 0.8;

	// Shift mutation
	m_run_parameters.p_shift = 0.2;
	m_run_parameters.min_shift = -4;
	m_run_parameters.max_shift = 4;

	// Add/Remove mutation
	m_run_parameters.p_add = 0.1;
	m_run_parameters.p_remove = 0.1;
	m_run_parameters.together_add_remove = false;
	m_run_parameters.number_columns_add_remove = 1;
	m_run_parameters.lower_limit_add_remove = 1;
	m_run_parameters.upper_limit_add_remove = m_run_parameters.x_max - 1;

	m_placement_results.resize(m_run_parameters.population_size);
}

PlacementAlgorithmEvolutionary::PlacementAlgorithmEvolutionary(
    EvolutionaryParameters run_parameters) :
    m_run_parameters(run_parameters)
{
	if (run_parameters.x_max > 127) {
		throw std::range_error("Too large value for x_max. Must be smaller than 128.");
	}
	m_placement_results.resize(m_run_parameters.population_size);
}

void PlacementAlgorithmEvolutionary::construct_neuron(
    PlacementResult& parallel_result, size_t x_max)
{
	// Current Placement Result
	AlgorithmResult& result_temp = parallel_result.result;
	NumberTopBottom& total_resources_build = parallel_result.resources_total_build;
	std::map<CompartmentOnNeuron, NumberTopBottom>& resources_constructed =
	    parallel_result.resources_build;

	// Empty Neuron for construction
	Neuron& neuron_constructed = parallel_result.neuron_build;
	neuron_constructed.clear();

	// Empty Resources for construction
	resources_constructed.clear();
	total_resources_build = NumberTopBottom();

	// Iterate over coordinate-system to find compartments with set switches and add to constructed
	// neuron and constructed resources
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (result_temp.coordinate_system.coordinate_system[y][x].compartment ==
			        CompartmentOnNeuron() &&
			    result_temp.coordinate_system.connected(x, y)) {
				// Add Compartment to Neuron
				Compartment temp_compartment;
				CompartmentOnNeuron temp_descriptor =
				    neuron_constructed.add_compartment(temp_compartment);
				// Assign ID to Coordinate System
				NumberTopBottom number_neuron_circuits =
				    result_temp.coordinate_system.assign_compartment_adjacent(
				        x, y, temp_descriptor);
				// Add allocated resources to resource map
				resources_constructed.emplace(temp_descriptor, number_neuron_circuits);
				total_resources_build += number_neuron_circuits;
			}
		}
	}

	// Add Compartment Connections
	CompartmentConnectionConductance connection_temp;

	// For each neuron circuits search for connected neighbours
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			// Find all connected compartments
			if (!result_temp.coordinate_system.coordinate_system[y][x].compartment) {
				continue;
			}
			for (auto neighbour_coordinates :
			     result_temp.coordinate_system.connected_shared_conductance(x, y)) {
				// Check if already connected on neuron
				if (neuron_constructed.neighbour(
				        result_temp.coordinate_system.coordinate_system[y][x].compartment.value(),
				        result_temp.coordinate_system
				            .coordinate_system[neighbour_coordinates.second]
				                              [neighbour_coordinates.first]
				            .compartment.value())) {
					continue;
				}
				// Check for connection with itself
				if (result_temp.coordinate_system.coordinate_system[y][x].compartment ==
				    result_temp.coordinate_system
				        .coordinate_system[neighbour_coordinates.second]
				                          [neighbour_coordinates.first]
				        .compartment) {
					continue;
				}
				// Add connection
				neuron_constructed.add_compartment_connection(
				    result_temp.coordinate_system.coordinate_system[y][x].compartment.value(),
				    result_temp.coordinate_system
				        .coordinate_system[neighbour_coordinates.second]
				                          [neighbour_coordinates.first]
				        .compartment.value(),
				    connection_temp);
			}
		}
	}
}
double PlacementAlgorithmEvolutionary::fitness_number_compartments(
    PlacementResult const& parallel_result, Neuron const& neuron) const
{
	double compartments_constructed = parallel_result.neuron_build.num_compartments();
	double compartments = neuron.num_compartments();

	std::stringstream ss;
	ss << "Number of required compartments: " << compartments
	   << "\nNumber of constructed compartments: " << compartments_constructed
	   << "\nResulting fitness: "
	   << 1 - (std::abs(compartments - compartments_constructed) /
	           std::max(compartments, compartments_constructed));
	LOG4CXX_TRACE(m_logger, ss.str());

	return 1 - (std::abs(compartments - compartments_constructed) /
	            std::max(compartments, compartments_constructed));
}

double PlacementAlgorithmEvolutionary::fitness_number_compartment_connections(
    PlacementResult const& parallel_result, Neuron const& neuron) const
{
	double connections_constructed = parallel_result.neuron_build.num_compartment_connections();
	double connections = neuron.num_compartment_connections();

	std::stringstream ss;
	ss << "Number of required connections: " << connections
	   << "\nNumber of constructed connections: " << connections_constructed;

	ss << "\nResulting fitness: ";
	if (connections == 0 && connections_constructed == 0) {
		ss << 1;
		LOG4CXX_TRACE(m_logger, ss.str());
		return 1;
	}

	ss << 1 - (std::abs(connections - connections_constructed) /
	           std::max(connections, connections_constructed));
	LOG4CXX_TRACE(m_logger, ss.str());

	return 1 - (std::abs(connections - connections_constructed) /
	            std::max(connections, connections_constructed));
}
double PlacementAlgorithmEvolutionary::fitness_resources_total(
    PlacementResult const& parallel_result, ResourceManager const& resources) const
{
	NumberTopBottom constructed_resources_total = parallel_result.resources_total_build;
	NumberTopBottom required_resources_total = resources.get_total();


	// Calculate weighted fitnesses by difference of total resource reqirements
	double resources_required = static_cast<double>(required_resources_total.number_total);
	double resources_constructed = static_cast<double>(constructed_resources_total.number_total);
	double resources_buffer = 0.3 * resources_required;

	std::stringstream ss;
	ss << "Number of required resources: " << resources_required
	   << "\nNumber of allocated resources: " << resources_constructed;

	ss << "\nResutling fitness: ";
	if (resources_constructed < resources_required) {
		ss << 1 - (std::abs(resources_required - resources_constructed) /
		           std::max(resources_required, resources_constructed));
		LOG4CXX_TRACE(m_logger, ss.str());

		return 1 - (std::abs(resources_required - resources_constructed) /
		            std::max(resources_required, resources_constructed));
	} else if (resources_constructed < resources_required + resources_buffer) {
		ss << 1 + 1;
		LOG4CXX_TRACE(m_logger, ss.str());

		return 1 + 1;
	} else {
		ss << 1 + 1 -
		          (std::abs(resources_required - resources_constructed) /
		           std::max(resources_required, resources_constructed));
		LOG4CXX_TRACE(m_logger, ss.str());

		return 1 + 1 -
		       (std::abs(resources_required - resources_constructed) /
		        std::max(resources_required, resources_constructed));
	}
}
double PlacementAlgorithmEvolutionary::fitness_isomorphism(
    PlacementResult const& parallel_result,
    Neuron const& neuron,
    ResourceManager const& resources) const
{
	double number_difference =
	    isomorphism_resources_subgraph(
	        neuron, parallel_result.neuron_build, resources, parallel_result.resources_build)
	        .first;

	std::stringstream ss;
	ss << "Number of unmappable compartments: " << number_difference << " of "
	   << neuron.num_compartments() << " total compartments\nResulting fitness: "
	   << 1 - (number_difference / neuron.num_compartments());
	LOG4CXX_TRACE(m_logger, ss.str());

	return (1 - (number_difference / neuron.num_compartments()));
}
double PlacementAlgorithmEvolutionary::fitness_recording(
    PlacementResult const& parallel_result, ResourceManager const& resources) const
{
	std::set<CompartmentOnNeuron> even_parity =
	    parallel_result.result.coordinate_system.even_parity();
	std::set<CompartmentOnNeuron> odd_parity =
	    parallel_result.result.coordinate_system.odd_parity();

	double recordable = 0;
	double not_recordable = 0;

	for (auto record_pair : resources.get_recordable_pairs()) {
		if ((even_parity.contains(record_pair.first) && odd_parity.contains(record_pair.second)) ||
		    (even_parity.contains(record_pair.second) && odd_parity.contains(record_pair.first))) {
			recordable++;
		} else {
			not_recordable++;
		}
	}

	return (recordable) / (recordable + not_recordable);
}


void PlacementAlgorithmEvolutionary::build_coordinate_system(
    PlacementResult& parallel_result, size_t x_max, std::vector<bool> switch_configuration)
{
	AlgorithmResult& result = parallel_result.result;
	result.coordinate_system.clear();

	if (switch_configuration.size() != 9 * x_max) {
		throw std::length_error(
		    "Number of switch configurations does not match size of coordinate system.");
	}

	size_t y = 0;
	for (size_t i = 0; i < x_max; i++) {
		result.coordinate_system.coordinate_system[0][i].switch_top_bottom =
		    switch_configuration[9 * i];
		result.coordinate_system.coordinate_system[1][i].switch_top_bottom =
		    switch_configuration[9 * i];

		result.coordinate_system.coordinate_system[y][i].switch_right =
		    switch_configuration[9 * i + 1];

		result.coordinate_system.coordinate_system[y][i].switch_circuit_shared =
		    switch_configuration[9 * i + 2];

		result.coordinate_system.coordinate_system[y][i].switch_circuit_shared_conductance =
		    (switch_configuration[9 * i + 3] && !switch_configuration[9 * i + 2]);

		result.coordinate_system.coordinate_system[y][i].switch_shared_right =
		    switch_configuration[9 * i + 4];

		y = 1 - y; // Switch row

		result.coordinate_system.coordinate_system[y][i].switch_right =
		    switch_configuration[9 * i + 5];

		result.coordinate_system.coordinate_system[y][i].switch_circuit_shared =
		    switch_configuration[9 * i + 6];

		result.coordinate_system.coordinate_system[y][i].switch_circuit_shared_conductance =
		    (switch_configuration[9 * i + 7] && !switch_configuration[9 * i + 6]);

		result.coordinate_system.coordinate_system[y][i].switch_shared_right =
		    switch_configuration[9 * i + 8];
	}
	result.coordinate_system.clear_invalid_connections(x_max);
}

double PlacementAlgorithmEvolutionary::fitness(
    PlacementResult& parallel_result,
    size_t x_max,
    Neuron const& neuron,
    ResourceManager const& resources)
{
	construct_neuron(parallel_result, x_max);

	double fitness_num_compartments = fitness_number_compartments(parallel_result, neuron);

	double fitness_num_compartments_connections =
	    fitness_number_compartment_connections(parallel_result, neuron);

	double fitness_resources = fitness_resources_total(parallel_result, resources);

	double fitness_isomorphic = fitness_isomorphism(parallel_result, neuron, resources);

	double fitness_recordable = fitness_recording(parallel_result, resources);

	double fitness = 100 * fitness_num_compartments + 100 * fitness_num_compartments_connections +
	                 50 * fitness_resources + 200 * fitness_isomorphic + 10 * fitness_recordable;

	return fitness;
}


AlgorithmResult PlacementAlgorithmEvolutionary::run(
    CoordinateSystem const&, Neuron const& neuron, ResourceManager const& resources)
{
	m_result_final.finished = false;
	LOG4CXX_DEBUG(m_logger, "Starting evolutionary run.");

	LOG4CXX_TRACE(m_logger, m_run_parameters);

	// Random number generators
	std::mt19937 gen(m_run_parameters.seed);

	// Evolutionary Objects
	typedef EoChrom<bool, double> Chrom;
	typedef EoPopulation<Chrom> Population;
	SelectionNBest<Chrom> select_best(m_run_parameters.number_hall_of_fame, false);
	Tournament<Chrom> tournament(
	    m_run_parameters.tournament_contestants,
	    m_run_parameters.population_size - m_run_parameters.number_hall_of_fame, gen);

	MatingExchangeBlocks<Chrom> mating(
	    9 * m_run_parameters.x_max, m_run_parameters.mating_block_size, m_run_parameters.p_mate,
	    gen);
	MutationBitFlip<Chrom> mutator(
	    m_run_parameters.p_mutate_individual, m_run_parameters.p_mutate_gene, gen);
	MutationBitFlip<Chrom> starting_mutator(1, m_run_parameters.p_mutate_initial, gen);
	MutationShiftRound<Chrom> shift(
	    m_run_parameters.p_shift, m_run_parameters.min_shift, m_run_parameters.max_shift, gen);
	MutationAddColumns<Chrom> add_columns(
	    m_run_parameters.p_add, m_run_parameters.number_columns_add_remove,
	    m_run_parameters.together_add_remove, m_run_parameters.lower_limit_add_remove,
	    m_run_parameters.upper_limit_add_remove, gen);
	MutationRemoveColumns<Chrom> remove_columns(
	    m_run_parameters.p_remove, m_run_parameters.number_columns_add_remove,
	    m_run_parameters.together_add_remove, m_run_parameters.lower_limit_add_remove,
	    m_run_parameters.upper_limit_add_remove, gen);

	// Setting up initial population of chromes
	Population population;
	for (size_t i = 0; i < m_run_parameters.population_size; i++) {
		Chrom chrom(9 * m_run_parameters.x_max);
		chrom.fitness(0);
		population.push_back(chrom);
	}

	// Outer loop
	hate::Timer execution_timer;
	size_t generation_counter = 0;
	AlgorithmResult best_result_in_pop;
	Population hall_of_fame_population;
	Population temp_population;
	temp_population.resize(population.size());

	size_t run_counter = 1;
	while (run_counter <= m_run_parameters.run_limit && !m_result_final.finished) {
		run_counter++;
		// Mutating to starting configuration
		starting_mutator(population);

		hate::Timer timer;
		while (!valid(best_result_in_pop, m_run_parameters.x_max, neuron, resources) &&
		       static_cast<size_t>(timer.get_s()) < m_run_parameters.time_limit) {
			generation_counter++;

			LOG4CXX_TRACE(m_logger, "Generation: " + std::to_string(generation_counter));

			select_best(population, hall_of_fame_population);

			tournament(population, temp_population);
			std::swap(population, temp_population);

			mating(population, temp_population);
			std::swap(population, temp_population);

			mutator(population);

			shift(population);

			add_columns(population);
			remove_columns(population);

			// After channging the population without the #number_hall_of_fame put them in one
			// population again for next round
			for (auto chrom : hall_of_fame_population) {
				population.push_back(chrom);
			}
			hall_of_fame_population.clear();

			auto lambda_fitness = [this, &population, &neuron, &resources](size_t index) -> void {
				for (size_t i = index; i < index + m_run_parameters.parallel_threads; i++) {
					PlacementResult& parallel_result = m_placement_results.at(i);

					build_coordinate_system(
					    parallel_result, m_run_parameters.x_max, population.at(i));
					double temp_fitness =
					    fitness(parallel_result, m_run_parameters.x_max, neuron, resources);
					population[i].fitness(temp_fitness);
				}
			};

			// Calculating the fitness of each chrom in the population
			std::vector<std::future<void>> void_results;
			for (size_t i = 0; i < population.size() / m_run_parameters.parallel_threads; i++) {
				void_results.push_back(std::async(
				    std::launch::async, lambda_fitness, m_run_parameters.parallel_threads * i));
			}

			for (auto& void_result : void_results) {
				void_result.wait();
			}

			// Find best element and validate in loop
			build_coordinate_system(
			    m_placement_results.at(0), m_run_parameters.x_max,
			    population.get_best_individual());
			best_result_in_pop = m_placement_results.at(0).result;
			best_result_in_pop.coordinate_system.clear_empty_connections(m_run_parameters.x_max);
			m_fitness_best_in_pop.push_back(population.get_best_individual().fitness());
			LOG4CXX_DEBUG(
			    m_logger, "Fitness of best individual" +
			                  std::to_string(population.get_best_individual().fitness()));

			// Add best in generation to list of best results per generation
			m_best_in_pop.push_back(best_result_in_pop);
		}
	}

	double exec_time = execution_timer.get_ms();

	std::stringstream ss;
	ss << "Algorithm run time: " << exec_time / 1000 << "[s]"
	   << "\nGenerations passed: " << generation_counter << "\nGenerations per second: "
	   << static_cast<double>(generation_counter) * 1000 / exec_time;

	if (m_result_final.finished) {
		ss << "Solution found. Success.";
	} else {
		ss << "No solution found. Timeout.";
	}

	LOG4CXX_DEBUG(m_logger, ss.str());

	return m_result_final;
}

std::unique_ptr<PlacementAlgorithm> PlacementAlgorithmEvolutionary::clone() const
{
	auto new_algorithm = std::make_unique<PlacementAlgorithmEvolutionary>();
	new_algorithm->m_run_parameters = m_run_parameters;
	new_algorithm->reset();
	return new_algorithm;
}

void PlacementAlgorithmEvolutionary::reset()
{
	m_terminate_parallel = false;
	m_best_in_pop.clear();
	m_fitness_best_in_pop.clear();
	m_result_final = AlgorithmResult();
	m_placement_results.clear();
	m_placement_results.resize(m_run_parameters.population_size);
}


bool PlacementAlgorithmEvolutionary::valid(
    AlgorithmResult best_result,
    size_t x_max,
    Neuron const& neuron,
    ResourceManager const& resources)
{
	if (x_max > 128) {
		throw std::invalid_argument("x_max > 128, invalid");
	}

	// Check for empty Connections (Dead Ends)
	if (best_result.coordinate_system.has_empty_connections(128) ||
	    best_result.coordinate_system.has_double_connections(128)) {
		return false;
	}


	// Construct Neuron from State of Switches in Coordinate System
	Neuron neuron_constructed;
	std::map<CompartmentOnNeuron, NumberTopBottom> resources_constructed;
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (best_result.coordinate_system.coordinate_system[y][x].compartment ==
			        CompartmentOnNeuron() &&
			    best_result.coordinate_system.connected(x, y)) {
				// Add Compartment to Neuron
				Compartment temp_compartment;
				CompartmentOnNeuron temp_descriptor =
				    neuron_constructed.add_compartment(temp_compartment);
				// Assign ID to Coordinate System
				NumberTopBottom number_neuron_circuits =
				    best_result.coordinate_system.assign_compartment_adjacent(
				        x, y, temp_descriptor);
				// Add allocated resources to resource map
				resources_constructed.emplace(temp_descriptor, number_neuron_circuits);
			}
		}
	}

	// Check for number of compartments. This is an early return to avoid execution of isomorphism.
	if (neuron_constructed.num_compartments() != neuron.num_compartments()) {
		return false;
	}
	LOG4CXX_TRACE(m_logger, "Validation: Number of compartments valid.");

	// Add Compartment Connections
	CompartmentConnectionConductance connection_temp;
	for (size_t x = 0; x < x_max && x < 128 - 1; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (best_result.coordinate_system.connected_right_shared(x, y) &&
			    !neuron_constructed.neighbour(
			        best_result.coordinate_system.coordinate_system[y][x].compartment.value(),
			        best_result.coordinate_system.coordinate_system[y][x + 1]
			            .compartment.value())) {
				neuron_constructed.add_compartment_connection(
				    best_result.coordinate_system.coordinate_system[y][x].compartment.value(),
				    best_result.coordinate_system.coordinate_system[y][x + 1].compartment.value(),
				    connection_temp);
			}
		}
	}

	// Check for number of compartment-connections. This is an early return to avoid execution of
	// isomorphism.
	if (neuron_constructed.num_compartment_connections() != neuron.num_compartment_connections()) {
		return false;
	}
	LOG4CXX_TRACE(m_logger, "Validation: Number of connections valid.");

	// Check for isomorphism
	std::pair<size_t, std::map<CompartmentOnNeuron, CompartmentOnNeuron>> isomorphism =
	    isomorphism_resources(neuron, neuron_constructed, resources, resources_constructed);
	if (isomorphism.first != 0) {
		return false;
	}
	LOG4CXX_TRACE(m_logger, "Validation: Isomorphism valid.");

	// Check for MADC-recordable pairs
	std::set<CompartmentOnNeuron> even_parity = best_result.coordinate_system.even_parity();
	std::set<CompartmentOnNeuron> odd_parity = best_result.coordinate_system.odd_parity();
	for (auto record_pair : resources.get_recordable_pairs()) {
		if (!(even_parity.contains(record_pair.first) && odd_parity.contains(record_pair.second)) &&
		    !(even_parity.contains(record_pair.second) && odd_parity.contains(record_pair.first))) {
			return false;
		}
	}


	// Assinging correct compartment-IDs to coordinate system
	for (size_t x = 0; x < x_max + 2; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (!best_result.coordinate_system.coordinate_system[y][x].compartment) {
				continue;
			}
			best_result.coordinate_system.coordinate_system[y][x].compartment =
			    isomorphism.second[best_result.coordinate_system.coordinate_system[y][x]
			                           .compartment.value()];
		}
	}

	// Assign result with temporary result
	best_result.finished = true;

	m_result_final = best_result;
	return true;
}


std::vector<CoordinateSystem> PlacementAlgorithmEvolutionary::get_best_in_pops() const
{
	std::vector<CoordinateSystem> bests;
	for (auto const& best : m_best_in_pop) {
		bests.push_back(best.coordinate_system);
	}
	return bests;
}
AlgorithmResult PlacementAlgorithmEvolutionary::get_final_result() const
{
	return m_result_final;
}
} // namespace grenade::vx::network::abstract