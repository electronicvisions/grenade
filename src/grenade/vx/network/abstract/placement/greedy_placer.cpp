#include "grenade/vx/network/abstract/placement/greedy_placer.h"

#include "grenade/common/edge_on_topology.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/input_data.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/population.h"
#include "grenade/common/projection_connector/static.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/network/abstract/calibration.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/mapped_topology_rewrite/placement.h"
#include "grenade/vx/network/abstract/mapped_topology_rewrite/routing.h"
#include "grenade/vx/network/abstract/population_cell/delay.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated_signed.h"
#include "grenade/vx/network/abstract/recorder/cadc.h"
#include "grenade/vx/network/abstract/recorder/madc.h"
#include "grenade/vx/network/abstract/recorder/pad.h"
#include "grenade/vx/network/abstract/recorder/spike.h"
#include "grenade/vx/network/abstract/resource_estimator/population.h"
#include "grenade/vx/network/connectum.h"
#include "grenade/vx/network/exception.h"
#include "grenade/vx/network/routing/portfolio_router.h"
#include "halco/hicann-dls/vx/v3/background.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/padi.h"
#include "hate/indent.h"
#include "hate/timer.h"
#include "pyhxcomm/vx/connection_handle.h"
#include <Python.h>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <boost/range/iterator_range_core.hpp>
#include <log4cxx/logger.h>
#include <pybind11/embed.h>

namespace grenade::vx::network::abstract {

GreedyPlacer::GreedyPlacer() :
    m_neuron_permutation(),
    m_background_source_permutation(),
    m_logger(log4cxx::Logger::getLogger("grenade.network.abstract.GreedyPlacer"))
{
	for (auto const atomic_neuron :
	     halco::common::iter_all<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>()) {
		m_neuron_permutation.push_back(atomic_neuron);
	}
	for (auto const background_source :
	     halco::common::iter_all<halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock>()) {
		m_background_source_permutation.push_back(background_source);
	}
}

void GreedyPlacer::set_neuron_permutation(NeuronPermutation value)
{
	if (std::set(value.begin(), value.end()).size() != value.size()) {
		throw std::invalid_argument("Neuron permutation is required to contain unique elements.");
	}
	m_neuron_permutation = std::move(value);
}

GreedyPlacer::NeuronPermutation const& GreedyPlacer::get_neuron_permutation() const
{
	return m_neuron_permutation;
}

void GreedyPlacer::set_background_source_permutation(BackgroundSourcePermutation value)
{
	if (std::set(value.begin(), value.end()).size() != value.size()) {
		throw std::invalid_argument(
		    "BackgroundSource permutation is required to contain unique elements.");
	}
	m_background_source_permutation = std::move(value);
}

GreedyPlacer::BackgroundSourcePermutation const& GreedyPlacer::get_background_source_permutation()
    const
{
	return m_background_source_permutation;
}

PlacementResult GreedyPlacer::operator()(grenade::common::LinkedTopology const& topology) const
{
	hate::Timer mapping_timer;
	using namespace grenade::common;

	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
	    available_neuron_circuits;

	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::vector<halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock>>
	    available_background_circuits;

	PlacementResult placement_result;

	for (auto const vertex_descriptor : topology.vertices()) {
		if (!dynamic_cast<PartitionedVertex const&>(topology.get(vertex_descriptor))
		         .get_execution_instance_on_executor()) {
			throw std::invalid_argument(
			    "Topology not fully mapped, execution instance id assignment incomplete.");
		}
		if (auto const population_ptr =
		        dynamic_cast<grenade::common::Population const*>(&topology.get(vertex_descriptor));
		    population_ptr) {
			if (auto const neuron_ptr =
			        dynamic_cast<LocallyPlacedNeuron const*>(&(population_ptr->get_cell()));
			    neuron_ptr) {
				if (!available_neuron_circuits.contains(
				        population_ptr->get_execution_instance_on_executor().value())) {
					available_neuron_circuits[population_ptr->get_execution_instance_on_executor()
					                              .value()] = m_neuron_permutation;
				}
				auto& local_available_neuron_circuits = available_neuron_circuits.at(
				    *population_ptr->get_execution_instance_on_executor());

				auto const& sequence = population_ptr->get_shape();

				for (size_t i = 0; i < sequence.size(); ++i) {
					bool placed = false;
					for (auto const& anchor : local_available_neuron_circuits) {
						try {
							halco::hicann_dls::vx::v3::LogicalNeuronOnDLS logical_neuron(
							    neuron_ptr->shape, anchor);


							auto const atomic_neurons = logical_neuron.get_atomic_neurons();
							if (!std::all_of(
							        atomic_neurons.begin(), atomic_neurons.end(),
							        [local_available_neuron_circuits](auto const& atomic_neuron) {
								        return std::find(
								                   local_available_neuron_circuits.begin(),
								                   local_available_neuron_circuits.end(),
								                   atomic_neuron) !=
								               local_available_neuron_circuits.end();
							        })) {
								continue;
							}

							placement_result.neuron_anchors[vertex_descriptor].push_back(anchor);
							for (auto const& atomic_neuron : atomic_neurons) {
								local_available_neuron_circuits.erase(std::find(
								    local_available_neuron_circuits.begin(),
								    local_available_neuron_circuits.end(), atomic_neuron));
							}
							placed = true;
							break;
						} catch (std::runtime_error const&) {
							// this can happen if we can't construct the logical neuron coordinate
							// with the given anchor, for example when the neuron then overlaps
							// across the two neuron blocks with internal inter-neuron-circuit
							// connectivity
							continue;
						}
					}
					if (!placed) {
						throw std::runtime_error("Placement unsuccessful.");
					}
				}
			} else if (auto const neuron_ptr =
			               dynamic_cast<PoissonSourceNeuron const*>(&(population_ptr->get_cell()));
			           neuron_ptr) {
				if (!available_background_circuits.contains(
				        *population_ptr->get_execution_instance_on_executor())) {
					available_background_circuits
					    [population_ptr->get_execution_instance_on_executor().value()] =
					        m_background_source_permutation;
				}
				auto& local_available_background_circuits = available_background_circuits.at(
				    *population_ptr->get_execution_instance_on_executor());

				auto const& sequence = population_ptr->get_shape();

				// we support populations of size <=256 and map them to one background generator per
				// hemisphere
				if (sequence.size() > 256) {
					// each circuit can only represent up to 256 sources
					throw std::runtime_error("Placement unsuccessful.");
				}
				if (!local_available_background_circuits.empty()) {
					for (auto const hemisphere :
					     halco::common::iter_all<halco::hicann_dls::vx::v3::HemisphereOnDLS>()) {
						placement_result.background_locations[vertex_descriptor][hemisphere] =
						    *local_available_background_circuits.begin();
					}
					local_available_background_circuits.erase(
					    local_available_background_circuits.begin());
				} else {
					throw std::runtime_error("Placement unsuccessful.");
				}
			}
		}
	}

	LOG4CXX_TRACE(m_logger, "Placed partitioned topology in " << mapping_timer.print() << ".");

	return placement_result;
}


} // namespace grenade::vx::network::abstract
