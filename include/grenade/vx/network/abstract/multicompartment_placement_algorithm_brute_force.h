#pragma once

#include "grenade/vx/network/abstract/multicompartment_placement_algorithm.h"
#include "hate/timer.h"
#include <atomic>
#include <fstream>
#include <future>
#include <iostream>
#include <math.h>

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) SYMBOL_VISIBLE PlacementAlgorithmBruteForce : public PlacementAlgorithm
{
	PlacementAlgorithmBruteForce();
	PlacementAlgorithmBruteForce(size_t timeout);

	AlgorithmResult run(
	    CoordinateSystem const& coordinate_system,
	    Neuron const& neuron,
	    ResourceManager const& resources);

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

	// Parameters for run
	// Number of configurations checked in parallel.
	size_t parallel_runs = 0;
	// Highest iterated neuron circuits x coordinate.
	size_t x_limit = 10;

private:
	AlgorithmResult run_parallel(
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    size_t parallel,
	    size_t x_max,
	    size_t index_min);

	void run_partial(
	    Neuron const& neuron,
	    ResourceManager const& resources,
	    size_t result_index,
	    size_t loop_limit);

	/**
	 * Validates if Placement in coordinate_system matches target neuron
	 * @param result_index Current configuration coordinate system with placed neuron in m_results
	 * @param x_max Upper limit to which coordinate system is checked for validity
	 * @param neuron Target neuron
	 * @param resources Required resources for neuron-placement
	 */
	bool GENPYBIND(hidden) valid(
	    size_t result_index, size_t x_max, Neuron const& neuron, ResourceManager const& resources);


	std::vector<AlgorithmResult> m_results;
	std::vector<AlgorithmResult> m_results_temp;

	// Termination variable for parallelisation
	std::atomic<bool> m_termintate_parallel;

	size_t m_timeout = 60; //[s]

	log4cxx::LoggerPtr m_logger;
};

} // namespace abstract
} // namespace grenade::vx::network