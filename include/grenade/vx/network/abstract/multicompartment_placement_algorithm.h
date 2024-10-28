#pragma once

#include "grenade/vx/network/abstract/multicompartment_coordinate_limits.h"
#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_result.h"
#include "grenade/vx/network/abstract/multicompartment_placement_coordinate_system.h"
#include "grenade/vx/network/abstract/multicompartment_resource_manager.h"
#include "halco/hicann-dls/vx/neuron.h"
#include <utility>
#include <boost/graph/vf2_sub_graph_iso.hpp>

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {


// Forward-Declaration Neuron
struct Neuron;


// PlacementAlgorithm Base Class
struct GENPYBIND(visible) SYMBOL_VISIBLE PlacementAlgorithm
{
	PlacementAlgorithm();

	/**
	 * Execute Placement Algorithm.
	 * @param coordinate_system Initial state of coordinate system.
	 * @param neuron Neuron to be placed.
	 * @param resources Resource-reqirements of the neuron.
	 */
	virtual AlgorithmResult run(
	    CoordinateSystem const& coordinate_system,
	    Neuron const& neuron,
	    ResourceManager const& resources) = 0;

	/**
	 * Check if the coordinate system is in a valid configuration for the given target neuron.
	 * @param coordiante_system Coordinate system with placed neuron configuration.
	 * @param neuron Target neuron
	 * @param resources Resource requirements of the target neuron.
	 */
	bool valid(
	    CoordinateSystem const& coordiante_system,
	    Neuron const& neuron,
	    ResourceManager const& resources);

	/**
	 * Clone the algorithm.
	 * Since PlacementAlgorithm is an abstract class it can not be passed by value. Therefore a
	 * pointer is returned.
	 * @return Unique pointer to copy of the algorithm.
	 */
	virtual std::unique_ptr<PlacementAlgorithm> clone() const = 0;

	/**
	 * Reset all members of the algorithm. Used during testing.
	 */
	virtual void reset() = 0;

protected:
	/**
	 * Return resource efficiency of a given placement configuration.
	 * The efficiency is calculated by the rate of neuron circuits used inside of the box limited by
	 * the furthest circuit on the left and the furthest circuit on the right.
	 * @param configuration Coordinate system with placed neuron configuration.
	 * @param neuron Target neuron.
	 * @param resouces Resource requirements of the target neuron.
	 *  */
	double resource_efficient(
	    CoordinateSystem const& configuration,
	    Neuron const& neuron,
	    ResourceManager const& resources) const;

	/**
	 * Isomorphism which considers resource requirements of both neurons.
	 * @param neuron One neuron for comparison.
	 * @param neuron_build Other neuron for comparison.
	 * @param resources Resource requirements for the first neuron.
	 * @param resources_build Resource requriements for the other neuron.
	 * @return Number of unmapped compartments and mapping of some/ all compartments.
	 */
	std::pair<size_t, std::map<CompartmentOnNeuron, CompartmentOnNeuron>> isomorphism_resources(
	    Neuron const& neuron,
	    Neuron const& neuron_build,
	    ResourceManager const& resources,
	    std::map<CompartmentOnNeuron, NumberTopBottom> const& resources_build) const;


	/**
	 * Isomorphism which considers resource requirements of both neurons and searches for matching
	 * subneurons.
	 * @param neuron One neuron for comparison.
	 * @param neuron_build Other neuron for comparison.
	 * @param resources Resource requirements for the first neuron.
	 * @param resources_build Resource requriements for the other neuron.
	 * @return Number of unmapped compartments and mapping of some/ all compartments.
	 */
	std::pair<size_t, std::map<CompartmentOnNeuron, CompartmentOnNeuron>>
	isomorphism_resources_subgraph(
	    Neuron const& neuron,
	    Neuron const& neuron_build,
	    ResourceManager const& resources,
	    std::map<CompartmentOnNeuron, NumberTopBottom> const& resources_build) const;

	// Convert Configuration CoordinateSystem into logical Compartments
	halco::hicann_dls::vx::LogicalNeuronCompartments convert_to_logical_compartments(
	    CoordinateSystem const& coordinate_system, Neuron const& neuron);

private:
	log4cxx::LoggerPtr logger;
};

} // namespace abstract
} // namespace grenade::vx::network