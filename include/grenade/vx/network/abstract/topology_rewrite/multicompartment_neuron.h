#pragma once
#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/topology_rewrite.h"
#include "grenade/vx/network/abstract/mapping/multicompartment_neuron.h"
#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/placement/algorithm.h"
#include "grenade/vx/network/abstract/population_cell/calibrated.h"
#include "hate/visibility.h"

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace grenade::vx::network::abstract {

/**
 * Rewrite for multi-compartment neuron to CalibratedNeuron.
 */
struct SYMBOL_VISIBLE MulticompartmentNeuronRewrite : public grenade::common::TopologyRewrite
{
	/**
	 * Construct topology rewrite operation targeting given topology.
	 * @param topology Linked topology
	 * @param placement_algorithm Placement algorithm to use
	 */
	MulticompartmentNeuronRewrite(
	    std::shared_ptr<grenade::common::LinkedTopology> topology,
	    std::unique_ptr<PlacementAlgorithm> placement_algorithm);

	virtual void operator()() const override;

private:
	std::unique_ptr<PlacementAlgorithm> m_placement_algorithm;

	/**
	 * Check that no interval of parameters contains more than one point (min == max).
	 */
	void check_is_single_operation_point(Neuron::ParameterSpace const& model_parameter_space) const;

	/**
	 * Construct environment for population.
	 * @param vertex_on_topology Vertex descriptor to construct for
	 * @param model_population Model population to construct for
	 */
	Environment construct_environment(
	    grenade::common::VertexOnTopology vertex_on_topology,
	    grenade::common::Population const& model_population) const;

	/**
	 * Construct calibrated locally-placed neuron.
	 * @param logical_neuron_compartments Local placement
	 * @param model_neuron Unplaced neuron
	 * @param model_parameter_space Unplaced neuron parameter space
	 * @param environment Environment of unplaced neuron
	 * @return Tuple of calibrated locally-placed neuron, map of atomic neuron indices per mechanism
	 * and compartment, reverse map of mechanism per atomic neuron index per compartment
	 */
	std::tuple<
	    CalibratedNeuron,
	    std::map<
	        grenade::common::CompartmentOnNeuron,
	        std::map<MechanismOnCompartment, std::set<size_t>>>,
	    std::map<grenade::common::CompartmentOnNeuron, std::map<size_t, MechanismOnCompartment>>>
	construct_calibrated_neuron(
	    halco::hicann_dls::vx::v3::LogicalNeuronCompartments const& logical_neuron_compartments,
	    Neuron const& model_neuron,
	    Neuron::ParameterSpace const& model_parameter_space,
	    Environment const& environment) const;

	/**
	 * Construct parameter space of locally-placed calibrated neuron.
	 * @param calibrated_neuron Locally-placed calibrated neuron
	 * @param logical_neuron_compartments Local placement
	 * @param local_placement Placement algorithm result
	 * @param model_neuron Unplaced model neuron
	 * @param model_parameter_space Parameter space of unplaced model neuron
	 * @param mechanism_on_atomic_neuron_placement Atomic neuron indices per compartment per
	 * mechanism
	 */
	CalibratedNeuron::ParameterSpace construct_calibrated_neuron_parameter_space(
	    CalibratedNeuron const& calibrated_neuron,
	    halco::hicann_dls::vx::v3::LogicalNeuronCompartments const& logical_neuron_compartments,
	    AlgorithmResult const& local_placement,
	    Neuron const& model_neuron,
	    Neuron::ParameterSpace const& model_parameter_space,
	    std::map<
	        grenade::common::CompartmentOnNeuron,
	        std::map<MechanismOnCompartment, std::set<size_t>>>&
	        mechanism_on_atomic_neuron_placement) const;

	/**
	 * Construct mapping between unplaced and locally-placed neuron.
	 * @param model_neuron Unplaced model neuron
	 * @param population_size Size of population
	 * @param logical_neuron_compartments Local placement
	 * @param mechanism_readout_placement Placement of mechanism readout part on atomic neuron index
	 * on compartment
	 */
	MulticompartmentNeuronMapping construct_mapping(
	    Neuron const& model_neuron,
	    size_t population_size,
	    halco::hicann_dls::vx::v3::LogicalNeuronCompartments const& logical_neuron_compartments,
	    std::map<
	        grenade::common::CompartmentOnNeuron,
	        std::map<size_t, MechanismOnCompartment>> const& mechanism_readout_placement) const;

	/**
	 * Replace unplaced population by locally-placed population in linked topology.
	 * @param vertex_on_topology Vertex descriptor to replace
	 * @param model_population Unplaced model neuron population
	 * @param mechanism_on_atomic_neuron_placement Atomic neuron indices per compartment per
	 * mechanism
	 * @param calibrated_neuron Locally-placed neuron
	 * @param calibrated_neuron_parameter_space Locally-placed neuron parameter space
	 * @param mapping Mapping between unplaced and locally-placed population
	 */
	void replace_vertex(
	    grenade::common::VertexOnTopology vertex_on_topology,
	    grenade::common::Population const& model_population,
	    std::map<
	        grenade::common::CompartmentOnNeuron,
	        std::map<MechanismOnCompartment, std::set<size_t>>> const&
	        mechanism_on_atomic_neuron_placement,
	    CalibratedNeuron&& calibrated_neuron,
	    CalibratedNeuron::ParameterSpace&& calibrated_neuron_parameter_space,
	    MulticompartmentNeuronMapping&& mapping) const;

	log4cxx::LoggerPtr m_logger;
};

} // namespace grenade::vx::network::abstract
