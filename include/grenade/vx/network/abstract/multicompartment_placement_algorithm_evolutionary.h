#pragma once

#include "grenade/vx/network/abstract/evolutionary/multicompartment_evolutionary_chromosome.h"
#include "grenade/vx/network/abstract/evolutionary/multicompartment_evolutionary_population.h"
#include "grenade/vx/network/abstract/multicompartment_evolutionary_helper.h"
#include "grenade/vx/network/abstract/multicompartment_evolutionary_parameters.h"
#include "grenade/vx/network/abstract/multicompartment_evolutionary_placement_result.h"
#include "grenade/vx/network/abstract/multicompartment_placement_algorithm.h"
#include "hate/timer.h"
#include <array>
#include <atomic>
#include <fstream>
#include <future>
#include <random>

namespace log4cxx {

class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;

} // namespace log4cxx

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) SYMBOL_VISIBLE PlacementAlgorithmEvolutionary : public PlacementAlgorithm
{
	// Runs Algorithm
	AlgorithmResult run(
	    CoordinateSystem const& coordinate_system,
	    Neuron const& neuron,
	    ResourceManager const& resources);

	PlacementAlgorithmEvolutionary();

	PlacementAlgorithmEvolutionary(EvolutionaryParameters run_parameters);

	/**
	 * Clone the algorithm. Only clones the initial configuration not the current state of the
	 * algorithm. New algorithm is in state as after reset(). Since PlacementAlgorithm is the
	 * abstract base class the algorithm is passed as a pointer. This allows to pass the algorithm
	 * polymorphically to functions.
	 * @return Unique pointer to copy of the algorithm.
	 */
	std::unique_ptr<PlacementAlgorithm> clone() const;

	/**
	 * Reset all members of the algorithm. Used during testing.
	 */
	void reset();

	// Convenience functions for testing
	std::vector<CoordinateSystem> get_best_in_pops() const;
	AlgorithmResult get_final_result() const;

private:
	/**
	 * Construct a neuron based on the state of the coordinate-system represented by the genome of
	 * an individual.
	 * @param placement_result Placement result containing the coordinate-system.
	 * @param x_max Upper limit in x-direction to check for connections.
	 */
	void construct_neuron(PlacementResult& placement_result, size_t x_max);

	/**
	 * Calculate fitness based on the number of compartments.
	 * Compares the number of compartments in the given placement result with the number of
	 * compartments on the target neuron.
	 * @param placement_result Placement result.
	 * @param neuron Target neuron.
	 */
	double fitness_number_compartments(
	    PlacementResult const& placement_result, Neuron const& neuron) const;

	/**
	 * Calculate fitness based on the number of connections.
	 * Compares the number of connections in the given placement result with the number of
	 * connnections on the target neuron.
	 * @param placement_result Placement result.
	 * @param neuron Target neuron.
	 */
	double fitness_number_compartment_connections(
	    PlacementResult const& placement_result, Neuron const& neuron) const;

	/**
	 * Calculate fitness based on the total number of allocated neuron circuits.
	 * Compares the number of neuron circuits with the number of neuron circuits required by the
	 * target neuron.
	 * If fewer neuron circuits are allocated the penalty is higher than for more circuits than
	 * required. The penaltiy for larger than required placements is required to avoid uncontrolled
	 * growth of the placement result. There is a buffer value that accounts for the fact that the
	 * placement results needs to allocate more neuron circuits than requried by the target neurons
	 * properties to make interconnections possible. Therfore the penalty for to large neurons is
	 * only applied if the number of allocated neuron circuits execeeds the required resources plus
	 * the buffer value.
	 * @param placement_result Placement result.
	 * @param resources Minimal required resources.
	 */
	double fitness_resources_total(
	    PlacementResult const& placement_result, ResourceManager const& resources) const;

	/**
	 * Calculate fitness based on what fraction of the neuron constructed from the placement result
	 * and the target neuron match topologically.
	 * @param parallal_result Placement result.
	 * @param neuron Target neuron.
	 * @param resources Minimal required resources.
	 */
	double fitness_isomorphism(
	    PlacementResult const& placement_result,
	    Neuron const& neuron,
	    ResourceManager const& resources) const;

	/**
	 * Construct coordinate system out of boolean vector retrieved by evolutionary algorithm.
	 * @param placement_result Result of a placement step.
	 * @param x_max Upper limit of the coordinate system
	 * @param individual Vector of boolean values that represent the configuration of the
	 * switches
	 */
	void build_coordinate_system(
	    PlacementResult& placement_result, size_t x_max, std::vector<bool> individual);

	/**
	 * Returns fitness of current placement.
	 * @param placement_result Result of a placement step.
	 * @param x_max Upper limit of the coordinate system.
	 * @param neuron Target neuron.
	 * @param resources Resources required by target neuron.
	 */
	double GENPYBIND(hidden) fitness(
	    PlacementResult& placement_result,
	    size_t x_max,
	    Neuron const& neuron,
	    ResourceManager const& resources);

	/**
	 * Validates if Placement in coordinate_system matches target neuron
	 * @param best_result Result of a placement step.
	 * @param x_max upper limit to which coordinate system is checked for validity
	 * @param neuron target neuron
	 * @param resources required resources for neuron-placement
	 */
	bool GENPYBIND(hidden) valid(
	    AlgorithmResult best_result,
	    size_t x_max,
	    Neuron const& neuron,
	    ResourceManager const& resources);

	EvolutionaryParameters m_run_parameters;


	std::atomic<bool> m_terminate_parallel;

	// Saving best in pop for testing purpose
	std::vector<AlgorithmResult> m_best_in_pop;
	std::vector<double> m_fitness_best_in_pop;

	AlgorithmResult m_result_final;

	std::vector<PlacementResult> m_placement_results;

	log4cxx::LoggerPtr m_logger;
};
} // namepsace abstract
} // namespace grenade::vx::network