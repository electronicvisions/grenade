#pragma once

#include "grenade/vx/network/abstract/multicompartment_placement_algorithm.h"
#include <array>
#include <atomic>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) PlacementAlgorithmEvolutionary : public PlacementAlgorithm
{
	// Runs Algorithm
	AlgorithmResult run(
	    CoordinateSystem const& coordinate_system,
	    Neuron const& neuron,
	    ResourceManager const& resources);

private:
	// Parts of Validation Step by step to determine overall fitness
	void construct_neuron(size_t result_index, size_t x_max);
	double fitness_number_compartments(size_t result_index, Neuron const& neuron) const;
	double fitness_number_compartment_connections(size_t result_index, Neuron const& neuron) const;
	double fitness_resources_total(size_t result_index, ResourceManager const& resources) const;
	double fitness_isomorphism(
	    size_t result_index, Neuron const& neuron, ResourceManager const& resources) const;

	/**
	 * Validates if Placement in coordinate_system matches target neuron
	 * @param result_index current configuration coordinate system with placed neuron in m_results
	 * @param x_max upper limit to which coordinate system is checked for validity
	 * @param neuron target neuron
	 * @param resources required resources for neuron-placement
	 */
	bool GENPYBIND(hidden) valid(
	    size_t result_index, size_t x_max, Neuron const& neuron, ResourceManager const& resources);

	/**
	 * Returns fitness of current placement
	 */
	double GENPYBIND(hidden) fitness(
	    size_t result_index, size_t x_max, Neuron const& neuron, ResourceManager const& resources);


	std::atomic<bool> m_terminate_parallel;
	static const size_t m_parallel = 1;


	std::array<AlgorithmResult, m_parallel> m_results;
	std::array<AlgorithmResult, m_parallel> m_results_temp;
	std::array<Neuron, m_parallel> m_constructed_neurons;
	std::array<std::map<CompartmentOnNeuron, NumberTopBottom>, m_parallel> m_constructed_resources;
	std::array<NumberTopBottom, m_parallel> m_constructed_resources_total;
	std::array<std::map<CompartmentOnNeuron, CompartmentOnNeuron>, m_parallel> m_descriptor_map;
};
} // namepsace abstract
} // namespace grenade::vx::network