#include "grenade/vx/network/abstract/multicompartment/neuron_generator.h"
#include <log4cxx/logger.h>

namespace grenade::vx::network::abstract {

NeuronGenerator::NeuronGenerator(size_t seed) :
    m_generator(seed), m_logger(log4cxx::Logger::getLogger("grenade.MC.NeuronGenerator"))
{
}

NeuronWithEnvironmentAndParameterSpace NeuronGenerator::generate()
{
	NeuronWithEnvironmentAndParameterSpace result;
	return result;
}

bool NeuronGenerator::cyclic(Neuron const& neuron) const
{
	LOG4CXX_TRACE(m_logger, "Checking for Cycle: " << *(neuron.compartments().first));
	CompartmentOnNeuron root = *(neuron.compartments().first);
	std::queue<CompartmentOnNeuron> compartment_queue;
	std::set<CompartmentOnNeuron> visited;
	compartment_queue.push(root);

	while (!compartment_queue.empty()) {
		CompartmentOnNeuron temp_compartment = compartment_queue.front();
		compartment_queue.pop();
		if (visited.contains(temp_compartment)) {
			LOG4CXX_TRACE(m_logger, "Cycle Detected.");
			return true;
		}
		visited.emplace(temp_compartment);

		for (auto it = neuron.adjacent_compartments(temp_compartment).first;
		     it != neuron.adjacent_compartments(temp_compartment).second; it++) {
			if (!visited.contains(*it)) {
				compartment_queue.push(*it);
				LOG4CXX_TRACE(m_logger, *it);
			}
		}
	}

	LOG4CXX_TRACE(m_logger, "No Cycle Detected.");
	return false;
}

bool NeuronGenerator::path(
    Neuron const& neuron,
    CompartmentOnNeuron const& compartment_a,
    CompartmentOnNeuron const& compartment_b) const
{
	std::set<CompartmentOnNeuron> visited;
	std::queue<CompartmentOnNeuron> next;

	next.push(compartment_a);
	CompartmentOnNeuron temp_compartment;
	while (!next.empty()) {
		temp_compartment = next.front();
		next.pop();

		if (temp_compartment == compartment_b) {
			return true;
		}

		visited.emplace(temp_compartment);

		for (auto neighbour :
		     boost::make_iterator_range(neuron.adjacent_compartments(temp_compartment))) {
			if (!visited.contains(neighbour)) {
				next.push(neighbour);
			}
		}
	}
	return false;
}


NeuronWithEnvironmentAndParameterSpace NeuronGenerator::generate(
    size_t num_compartments,
    size_t num_compartment_connections,
    size_t limit_synaptic_input,
    bool top_bottom_requirements,
    bool filter_loops)
{
	if (num_compartment_connections > (num_compartments * num_compartments - 1)) {
		throw std::invalid_argument("Too many Compartment Connections");
	}
	LOG4CXX_DEBUG(m_logger, "Generating Neuron.");

	// Result
	NeuronWithEnvironmentAndParameterSpace result;

	size_t run_counter = 0;
	do {
		run_counter++;
		result.neuron = Neuron();
		result.environment = Environment();
		result.parameter_space = Neuron::ParameterSpace();

		std::vector<CompartmentOnNeuron> compartment_ids;

		// Construct Neuron
		for (size_t i = 0; i < num_compartments; i++) {
			// Create Compartment
			Compartment temp_compartment;
			// Create Compartment-Parameter-Space
			Compartment::ParameterSpace compartment_parameter_space;
			// Create Mechanism
			MechanismSynapticInputCurrent temp_mechanism;
			// Create Mechanism-Parameter-Space
			MechanismSynapticInputCurrent::ParameterSpace mechanism_parameter_space;
			// Add mechanism to compartment
			MechanismOnCompartment mechanism_id = temp_compartment.add(temp_mechanism);
			compartment_parameter_space.mechanisms.set(mechanism_id, mechanism_parameter_space);
			// Add compartment to neuron
			CompartmentOnNeuron compartment_id = result.neuron.add_compartment(temp_compartment);
			compartment_ids.push_back(compartment_id);
			result.parameter_space.compartments.emplace(
			    compartment_id, compartment_parameter_space);
		}

		// Add information about synaptic input to environment
		std::uniform_int_distribution<> distribution_synaptic_inputs(0, limit_synaptic_input);
		for (size_t i = 0; i < num_compartments; i++) {
			SynapticInputEnvironmentCurrent temp_synaptic_input;

			temp_synaptic_input.number_of_inputs.number_total =
			    distribution_synaptic_inputs(m_generator);
			if (top_bottom_requirements) {
				std::uniform_int_distribution<> top(
				    0, temp_synaptic_input.number_of_inputs.number_total);
				temp_synaptic_input.number_of_inputs.number_top = top(m_generator);

				std::uniform_int_distribution<> bottom(
				    0, temp_synaptic_input.number_of_inputs.number_total -
				           temp_synaptic_input.number_of_inputs.number_top);
				temp_synaptic_input.number_of_inputs.number_bottom = bottom(m_generator);
			} else {
				temp_synaptic_input.number_of_inputs.number_top = 0;
				temp_synaptic_input.number_of_inputs.number_bottom = 0;
			}

			result.environment.add(compartment_ids.at(i), temp_synaptic_input);
		}


		// Random Connections between compartments
		CompartmentConnectionConductance connection;

		// List of all possible source target pairs (no permutations)
		std::vector<std::pair<size_t, size_t>> source_target_pairs;
		for (size_t i = 0; i < num_compartments; i++) {
			for (size_t j = i; j < num_compartments; j++) {
				if (i != j) {
					source_target_pairs.push_back(std::make_pair(i, j));
				}
			}
		}


		if (filter_loops) {
			// std::cout << "Generating Neuron without loops" << std::endl;
			for (size_t i = 0; i < num_compartment_connections; i++) {
				std::uniform_int_distribution<> distribution_connections(
				    0, source_target_pairs.size() - 1);
				size_t index = distribution_connections(m_generator);
				std::vector<std::pair<size_t, size_t>>::iterator index_iterator =
				    source_target_pairs.begin() + index;
				size_t source_index = source_target_pairs.at(index).first;
				size_t target_index = source_target_pairs.at(index).second;

				// If selected pair creates a cycle delete it and select another one
				while (source_target_pairs.size() > 1 &&
				       path(
				           result.neuron, compartment_ids.at(source_index),
				           compartment_ids.at(target_index))) {
					LOG4CXX_TRACE(m_logger, "Would create cycle. Selecting different pair.");
					source_target_pairs.erase(index_iterator);
					index = distribution_connections(m_generator) % source_target_pairs.size();
					index_iterator = source_target_pairs.begin() + index;
					source_index = source_target_pairs.at(index).first;
					target_index = source_target_pairs.at(index).second;
				}

				// Add connection to neuron and delete connection index pair
				result.neuron.add_compartment_connection(
				    compartment_ids.at(source_index), compartment_ids.at(target_index), connection);
				source_target_pairs.erase(source_target_pairs.begin() + index);
				// Remove all Pairs that have source as target to prevent loops
				for (size_t j = 0; j < source_target_pairs.size(); j++) {
					if (source_target_pairs.at(j).second == target_index) {
						source_target_pairs.erase(source_target_pairs.begin() + j);
					}
				}
			}
			assert(!cyclic(result.neuron));

		} else {
			for (size_t i = 0; i < num_compartment_connections; i++) {
				std::uniform_int_distribution<> distribution_connections(
				    0, source_target_pairs.size() - 1);
				size_t index = distribution_connections(m_generator);

				size_t source_index = source_target_pairs.at(index).first;
				size_t target_index = source_target_pairs.at(index).second;

				// Add connection to neuron and delete connection index pair
				result.neuron.add_compartment_connection(
				    compartment_ids.at(source_index), compartment_ids.at(target_index), connection);
				source_target_pairs.erase(source_target_pairs.begin() + index);
			}
		}
		LOG4CXX_DEBUG(
		    m_logger, "Connections placed sucessfully  [" << num_compartments << ", "
		                                                  << num_compartment_connections << "].");
	} while (!result.neuron.compartments_connected() || cyclic(result.neuron));

	return result;
}


} // namespace grenade::vx::network::abstract