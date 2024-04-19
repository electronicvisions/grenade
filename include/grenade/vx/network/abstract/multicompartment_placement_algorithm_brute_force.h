#pragma once

#include "grenade/vx/network/abstract/multicompartment_placement_algorithm.h"
#include "hate/timer.h"
#include <atomic>
#include <fstream>
#include <future>
#include <iostream>
#include <math.h>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) SYMBOL_VISIBLE PlacementAlgorithmBruteForce : public PlacementAlgorithm
{
	AlgorithmResult run(
	    CoordinateSystem const& coordinate_system,
	    Neuron const& neuron,
	    ResourceManager const& resources);

	// Parameters for run
	// Number of configurations checked in parallel.
	size_t parallel_runs = 0;
	// Highest iterated neuron circuits x coordinate.
	size_t x_limit = 10;

	size_t time_limit_single_thread = 60 * 30; //[s]
	size_t time_limit_multi_thread = 60 * 20;  //[s]

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
};

} // namespace abstract
} // namespace grenade::vx::network