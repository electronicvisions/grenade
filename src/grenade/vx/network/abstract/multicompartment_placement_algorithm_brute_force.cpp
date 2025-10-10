#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_brute_force.h"

namespace grenade::vx::network::abstract {


bool PlacementAlgorithmBruteForce::valid(
    size_t result_index, size_t x_max, Neuron const& neuron, ResourceManager const& resources)
{
	if (x_max > 128) {
		throw std::invalid_argument("x_max > 128, invalid");
	}

	// Final result to assign compartments
	AlgorithmResult& result = m_results.at(result_index);
	// Temporary result to perform tests on
	AlgorithmResult& result_temp = m_results_temp.at(result_index);
	result_temp = result;

	// Check for empty Connections, double connections (intern + extern) and short circuiting of two
	// compartments via the shared line
	if (result_temp.coordinate_system.has_empty_connections(128) ||
	    result_temp.coordinate_system.has_double_connections(128) ||
	    result_temp.coordinate_system.short_circuit(128)) {
		return false;
	}


	// Construct Neuron from State of Switches in Coordinate System
	Neuron neuron_constructed;
	std::map<CompartmentOnNeuron, NumberTopBottom> resources_constructed;
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (!result_temp.coordinate_system.coordinate_system[y][x].compartment &&
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
			}
		}
	}


	// Check for number of compartments
	if (neuron_constructed.num_compartments() != neuron.num_compartments()) {
		return false;
	}


	// Add Compartment Connections
	CompartmentConnectionConductance connection_temp;
	for (size_t x = 0; x < x_max && x < 128 - 1; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (!result_temp.coordinate_system.coordinate_system[y][x].compartment ||
			    !result_temp.coordinate_system.coordinate_system[y][x + 1].compartment) {
				continue;
			}
			if (result_temp.coordinate_system.connected_right_shared(x, y) &&
			    !neuron_constructed.neighbour(
			        result_temp.coordinate_system.coordinate_system[y][x].compartment.value(),
			        result_temp.coordinate_system.coordinate_system[y][x + 1]
			            .compartment.value())) {
				neuron_constructed.add_compartment_connection(
				    result_temp.coordinate_system.coordinate_system[y][x].compartment.value(),
				    result_temp.coordinate_system.coordinate_system[y][x + 1].compartment.value(),
				    connection_temp);
			}
		}
	}

	// Check for number of compartment-connections
	if (neuron_constructed.num_compartment_connections() != neuron.num_compartment_connections()) {
		return false;
	}

	// Check for isomorphism
	std::pair<size_t, std::map<CompartmentOnNeuron, CompartmentOnNeuron>> isomorphism =
	    isomorphism_resources(neuron, neuron_constructed, resources, resources_constructed);
	if (isomorphism.first != 0) {
		return false;
	}

	// Assing correct compartment-IDs to coordinate system
	for (size_t x = 0; x < x_max; x++) {
		for (size_t y = 0; y < 2; y++) {
			if (!result_temp.coordinate_system.coordinate_system[y][x].compartment) {
				continue;
			}
			result_temp.coordinate_system.coordinate_system[y][x].compartment =
			    isomorphism.second[result_temp.coordinate_system.coordinate_system[y][x]
			                           .compartment.value()];
		}
	}

	// Assign result with temporary result
	result_temp.finished = true;
	result = result_temp;
	return true;
}


AlgorithmResult PlacementAlgorithmBruteForce::run(
    CoordinateSystem const& coordinate_system,
    Neuron const& neuron,
    ResourceManager const& resources)
{
	hate::Timer timer_total;
	m_results.push_back(AlgorithmResult());
	m_results_temp.push_back(AlgorithmResult());
	AlgorithmResult& result = m_results.at(0);
	result.coordinate_system = coordinate_system;


	// If Parallel Runs requested
	if (parallel_runs != 0) {
		std::cout << "Parallel Run" << std::endl;
		run_parallel(neuron, resources, parallel_runs, x_limit, 5);
		for (auto temp_result : m_results) {
			if (temp_result.finished) {
				std::cout << "Final Result" << std::endl;
				for (size_t x = 0; x < 20; x++) {
					std::cout << temp_result.coordinate_system.coordinate_system[0][x].get_status()
					          << ",";
				}
				std::cout << std::endl;
				for (size_t x = 0; x < 20; x++) {
					std::cout << temp_result.coordinate_system.coordinate_system[1][x].get_status()
					          << ",";
				}
				std::cout << std::endl;
				return temp_result;
			}
		}
		return result;
	}
	std::cout << "Single Thread Run" << std::endl;

	// Hard Coded Case: 1 compartment with 1 neuron circuit
	if (neuron.num_compartments() == 1 &&
	    resources.get_config(*(neuron.compartment_iterators().first)).number_total == 1) {
		result.coordinate_system.coordinate_system[0][0].compartment =
		    *(neuron.compartment_iterators().first);
		result.finished = true;
		m_results.push_back(result);
		m_results_temp.push_back(result);
		return result;
	}

	// Put pointer to SwitchConfigs in iterable order
	std::vector<UnplacedNeuronCircuit*> config_pointers;
	for (size_t x = 0; x < 256; x++) {
		for (size_t y = 0; y < 2; y++) {
			config_pointers.push_back(&result.coordinate_system.coordinate_system[y][x]);
		}
	}


	size_t loop_counter = 0;
	size_t index_max = 0;
	while (!valid(0, (index_max / 2) + 2, neuron, resources)) {
		loop_counter++;
		// Increment first smallest Element till maximum than Increment second smallest and go
		// through smallest again
		size_t index = 0;
		while (index < config_pointers.size()) {
			if (++(*(config_pointers.at(index)))) {
				index = 0;
				break;
			} else {
				*(config_pointers.at(index)) = UnplacedNeuronCircuit();
				index++;
				if (index >= index_max) {
					index_max++;
				}
			}
		}
		if ((size_t) timer_total.get_s() > time_limit_single_thread) {
			std::cout << "Time Limit reached: Brute Force Terminated" << std::endl;
			result.finished = true;
			result.coordinate_system = CoordinateSystem();
			break;
		}
	}

	std::cout << "____________________Finished: Number of Iterations:" << loop_counter
	          << " in Time: " << timer_total.get_ms() << "[ms]____________________" << std::endl;

	/*Write Timers to file
	std::ofstream file;
	file.open("Timers_Brute_Force.csv"); //, std::ios_base::app);
	for (size_t i = 0; i < timers.size(); i++) {
	    file << timers.at(i).validation << ";" << timers.at(i).empty << ";"
	         << timers.at(i).construction << ";" << timers.at(i).num_compartments << ";"
	         << timers.at(i).num_connections << ";" << timers.at(i).isomorphism << ";"
	         << timers.at(i).resources << "\n";
	}
	file.close();
	*/

	if (!m_results.back().finished) {
		m_results.back().coordinate_system = CoordinateSystem();
	}

	return m_results.back();
}


AlgorithmResult PlacementAlgorithmBruteForce::run_parallel(
    Neuron const& neuron,
    ResourceManager const& resources,
    size_t parallel,
    size_t x_max,
    size_t index_min)
{
	// Prepare starting states for parallel brute force
	AlgorithmResult result;

	// create new configuration
	result.coordinate_system = CoordinateSystem();
	result.finished = false;

	// Put pointer to SwitchConfigs in iterable order
	std::vector<UnplacedNeuronCircuit*> config_pointers;
	for (size_t x = 0; x < 256; x++) {
		for (size_t y = 0; y < 2; y++) {
			config_pointers.push_back(&result.coordinate_system.coordinate_system[y][x]);
		}
	}

	size_t index_max = 0;
	size_t outer_loop_counter = 0;

	/**
	 * Outer While loop creates #parallel threads that run parallel and waits for them to finish.
	 * If a result is found, the search area is covered or the time limit is reached the loop
	 * repeats.
	 */
	hate::Timer timer_outer_loop;
	while (!m_termintate_parallel) {
		std::cout << "----------------------------------------Loop "
		             "Iteration----------------------------------------"
		          << outer_loop_counter << std::endl;
		// Loop over all starting configurations
		for (size_t i = 0; i < parallel; i++) {
			std::cout << "Building starting State " << i << std::endl;
			// Iterate Starting State


			size_t index = index_min;
			while ((index < x_max * 2 + 1) && (index < config_pointers.size())) {
				if (++(*(config_pointers.at(index)))) {
					std::cout << "Iterate at index: " << index << std::endl;
					index = index_min;
					break;
				} else {
					*(config_pointers.at(index)) = UnplacedNeuronCircuit();
					index++;
					if (index >= index_max) {
						index_max++;
					}
					std::cout << "Increment Index" << std::endl;
				}
			}
			if (result.coordinate_system == m_results.back().coordinate_system) {
				throw std::invalid_argument("No new coordinate system created: Duplicate");
			}

			// TESTING: Output Configuration of Coordinate System
			std::cout << "Push Back Starting State" << std::endl;
			for (size_t x = 0; x < 20; x++) {
				std::cout << result.coordinate_system.coordinate_system[0][x].get_status() << ",";
			}
			std::cout << std::endl;
			for (size_t x = 0; x < 20; x++) {
				std::cout << result.coordinate_system.coordinate_system[1][x].get_status() << ",";
			}
			std::cout << std::endl;

			// Put starting states in result vector
			if (outer_loop_counter == 0) {
				m_results.push_back(result);
			} else {
				m_results.at(i) = result;
			}
		}

		// Calculate Number of iterations for each thread
		size_t loop_limit = pow(24, index_min - 1); // Loop over first cicuits

		// Declare lambda_function for parallelisation call
		auto const lamda_function = [this](
		                                Neuron const* neuron, ResourceManager const* resources,
		                                size_t i, size_t loop_limit) {
			this->run_partial(*neuron, *resources, i, loop_limit);
		};

		std::vector<std::future<void>> void_results;
		// TO-DO: Run Algorithms
		for (size_t i = 0; i < parallel; i++) {
			void_results.push_back(
			    std::async(std::launch::async, lamda_function, &neuron, &resources, i, loop_limit));
		}

		for (auto& void_result : void_results) {
			void_result.wait();
		}


		// Time Limit
		if ((size_t) timer_outer_loop.get_s() > time_limit_multi_thread) {
			m_termintate_parallel = true;
			break;
		}
		outer_loop_counter++;
	}
	for (auto temp_result : m_results) {
		if (temp_result.finished) {
			return temp_result;
		}
	}
	result.coordinate_system = CoordinateSystem();
	result.finished = false;
	return result;
}

void PlacementAlgorithmBruteForce::run_partial(
    Neuron const& neuron, ResourceManager const& resources, size_t result_index, size_t loop_limit)
{
	// std::cout << "Thread " << result_index << " started" << std::endl;
	AlgorithmResult& result = m_results.at(result_index);

	// Put pointer to SwitchConfigs in iterable order
	std::vector<UnplacedNeuronCircuit*> config_pointers;
	for (size_t x = 0; x < 256; x++) {
		for (size_t y = 0; y < 2; y++) {
			config_pointers.push_back(&result.coordinate_system.coordinate_system[y][x]);
		}
	}

	// Hard Coded Case: 1 compartment with 1 neuron circuit
	if (neuron.num_compartments() == 1 &&
	    resources.get_config(*(neuron.compartment_iterators().first)).number_total == 1) {
		result.coordinate_system.coordinate_system[0][0].compartment =
		    *(neuron.compartment_iterators().first);
		result.finished = true;
		m_results.push_back(result); // TO-DO Remove?
	}


	size_t index_max = 0;
	size_t loop_counter = 0;
	while (!valid(result_index, (index_max / 2) + 2, neuron, resources)) {
		loop_counter++;
		// Increment first smallest Element till maximum than Increment second smallest and go
		// through smallest again
		size_t index = 0;
		while (index < config_pointers.size()) {
			if (++(*(config_pointers.at(index)))) {
				index = 0;
				break;
			} else {
				*(config_pointers.at(index)) = UnplacedNeuronCircuit();
				index++;
				if (index >= index_max) {
					index_max++;
				}
			}
		}


		if (loop_counter > loop_limit) {
			// std::cout << "Limit reached: Thread " << result_index << " terminated" << std::endl;
			result.finished = false;
			result.coordinate_system = CoordinateSystem();
			break;
		}
	}
	if (m_results.at(result_index).finished) {
		std::cout << "Thread: " << result_index << " found valid result" << std::endl;
	}
}


} // namespace grenade::vx::network::abstract