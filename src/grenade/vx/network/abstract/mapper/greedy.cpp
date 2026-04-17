#include "grenade/vx/network/abstract/mapper/greedy.h"

#include "grenade/common/edge_on_topology.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/input_data.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/population.h"
#include "grenade/common/projection_connector/static.h"
#include "grenade/common/topology_rewrite/add_linked_topology.h"
#include "grenade/common/topology_rewrite/execution_instance.h"
#include "grenade/common/topology_rewrite/identity_replacement.h"
#include "grenade/common/topology_rewrite/population.h"
#include "grenade/common/topology_rewrite/projection.h"
#include "grenade/common/topology_rewrite/recorder.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/network/abstract/calibration.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/mapped_topology_rewrite/placement.h"
#include "grenade/vx/network/abstract/mapped_topology_rewrite/routing.h"
#include "grenade/vx/network/abstract/plasticity_rule.h"
#include "grenade/vx/network/abstract/population_cell/delay.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated_signed.h"
#include "grenade/vx/network/abstract/recorder/cadc.h"
#include "grenade/vx/network/abstract/recorder/madc.h"
#include "grenade/vx/network/abstract/recorder/pad.h"
#include "grenade/vx/network/abstract/recorder/spike.h"
#include "grenade/vx/network/abstract/resource_estimator/population.h"
#include "grenade/vx/network/abstract/topology_rewrite/delay_cell.h"
#include "grenade/vx/network/abstract/topology_rewrite/plasticity_rule.h"
#include "grenade/vx/network/abstract/topology_rewrite/population_execution_instance_transition.h"
#include "grenade/vx/network/abstract/topology_rewrite/uncalibrated_signed_synapse.h"
#include "grenade/vx/network/connectum.h"
#include "grenade/vx/network/exception.h"
#include "grenade/vx/network/routing/greedy_router.h"
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

GreedyMapper::GreedyMapper() :
    m_placer(),
    m_router(std::make_shared<routing::GreedyRouter>()),
    m_logger(log4cxx::Logger::getLogger("grenade.network.abstract.GreedyMapper"))
{
}

void GreedyMapper::set_neuron_permutation(NeuronPermutation value)
{
	m_placer.set_neuron_permutation(std::move(value));
}

GreedyMapper::NeuronPermutation const& GreedyMapper::get_neuron_permutation() const
{
	return m_placer.get_neuron_permutation();
}

void GreedyMapper::set_background_source_permutation(BackgroundSourcePermutation value)
{
	m_placer.set_background_source_permutation(std::move(value));
}

GreedyMapper::BackgroundSourcePermutation const& GreedyMapper::get_background_source_permutation()
    const
{
	return m_placer.get_background_source_permutation();
}

void GreedyMapper::set_router(std::shared_ptr<routing::Router> router)
{
	m_router = std::move(router);
}


grenade::common::LinkedTopology GreedyMapper::operator()(
    std::shared_ptr<grenade::common::Topology const> topology,
    Calibration const& calibration,
    grenade::vx::execution::JITGraphExecutor& executor) const
{
	using namespace grenade::common;
	auto mapped_topology = std::make_shared<LinkedTopology>(topology);
	assert(topology);

	// partitioning-compatibility topology between model and partitioned topology
	//
	// This linked topology implements a mapping from parts of the model topology which can't be
	// partitioned to a representation, which can be partitioned.
	// Examples are: signed -> unsigned synapses, unplaced multi-compartment neurons ->
	// locally-placed multi-compartment neurons
	{
		hate::Timer copy_timer;
		IdentityReplacementTopologyRewrite copy_rewrite(mapped_topology);
		copy_rewrite();
		LOG4CXX_TRACE(
		    m_logger,
		    "Copied model topology to compatible topology in " << copy_timer.print() << ".");
	}

	{
		UncalibratedSignedSynapseRewrite uncalibrated_signed_synapse_rewrite(mapped_topology);
		uncalibrated_signed_synapse_rewrite();
	}

	// partitioned topology
	{
		AddLinkedTopologyRewrite add_linked_topology(mapped_topology);
		add_linked_topology();
	}

	{
		hate::Timer copy_timer;
		IdentityReplacementTopologyRewrite copy_rewrite(mapped_topology);
		copy_rewrite();
		LOG4CXX_TRACE(
		    m_logger, "Copied compatible topology to partitionable topology in "
		                  << copy_timer.print() << ".");
	}

	{
		PopulationResourceEstimator population_resource_estimator(*mapped_topology, {});
		PopulationResourceEstimator::Resource population_system_resources(
		    m_placer.get_neuron_permutation().size(),
		    m_placer.get_background_source_permutation().size(), {2});

		ExecutionInstanceTopologyRewrite::SystemResources executor_system_resources;
		for (auto const& connection_on_executor : executor.contained_connections()) {
			executor_system_resources.emplace(connection_on_executor, population_system_resources);
		}

		hate::Timer population_rewrite_timer;
		PopulationTopologyRewrite population_rewrite(
		    population_resource_estimator, executor_system_resources, mapped_topology);
		population_rewrite();
		LOG4CXX_TRACE(
		    m_logger, "Partitioned populations in partitionable topology in "
		                  << population_rewrite_timer.print() << ".");

		hate::Timer delay_cell_rewrite_timer;
		DelayCellRewrite delay_cell_rewrite(mapped_topology);
		delay_cell_rewrite();
		LOG4CXX_TRACE(
		    m_logger, "Partitioned delay cell populations in partitionable topology in "
		                  << delay_cell_rewrite_timer.print() << ".");

		hate::Timer recorder_rewrite_timer;
		RecorderTopologyRewrite recorder_rewrite(mapped_topology);
		recorder_rewrite();
		LOG4CXX_TRACE(
		    m_logger, "Partitioned recorders in partitionable topology in "
		                  << recorder_rewrite_timer.print() << ".");

		hate::Timer plasticity_rule_rewrite_timer;
		PlasticityRuleRewrite plasticity_rule_rewrite(mapped_topology);
		plasticity_rule_rewrite();
		LOG4CXX_TRACE(
		    m_logger, "Partitioned plasticity rules in partitionable topology in "
		                  << plasticity_rule_rewrite_timer.print() << ".");

		hate::Timer projection_rewrite_timer;
		ProjectionTopologyRewrite projection_rewrite(mapped_topology);
		projection_rewrite();
		LOG4CXX_TRACE(
		    m_logger, "Partitioned projections in partitionable topology in "
		                  << projection_rewrite_timer.print() << ".");

		hate::Timer execution_instance_rewrite_timer;
		ExecutionInstanceTopologyRewrite execution_instance_rewrite(
		    population_resource_estimator, executor_system_resources, mapped_topology);
		execution_instance_rewrite();
		LOG4CXX_TRACE(
		    m_logger, "Assigned execution instances to partitionable topology in "
		                  << execution_instance_rewrite_timer.print() << ".");

		hate::Timer population_execution_instance_transition_rewrite_timer;
		PopulationExecutionInstanceTransitionRewrite
		    population_execution_instance_transition_rewrite(mapped_topology);
		population_execution_instance_transition_rewrite();
		LOG4CXX_TRACE(
		    m_logger, "Added spike recorder and external source neuron population at execution "
		              "instance transitions to partitionable topology in "
		                  << population_execution_instance_transition_rewrite_timer.print() << ".");
	}

	{
		hate::Timer check_valid_timer;
		if (!mapped_topology->valid()) {
			std::stringstream ss;
			ss << "Partitioning not successful:\n";
			ss << *mapped_topology;
			throw std::runtime_error(ss.str());
		}
		LOG4CXX_TRACE(
		    m_logger,
		    "Checked for validity of partitioned topology in " << check_valid_timer.print() << ".");
	}

	LOG4CXX_TRACE(m_logger, "Found partitioned: " << *mapped_topology << ".");

	// back end topology
	hate::Timer mapping_timer;
	auto placement_result = m_placer(*mapped_topology);

	{
		AddLinkedTopologyRewrite add_linked_topology(mapped_topology);
		add_linked_topology();
	}

	PlacementRewrite placement_rewrite(std::move(placement_result), mapped_topology);
	placement_rewrite();

	LOG4CXX_TRACE(m_logger, "Mapped partitioned topology in " << mapping_timer.print() << ".");

	// calibrate
	hate::Timer calibration_timer;
	calibration(*mapped_topology, executor);
	LOG4CXX_TRACE(m_logger, "Calibrated mapped topology in " << calibration_timer.print() << ".");

	// route
	hate::Timer network_routing_timer;
	if (!m_router) {
		throw std::logic_error("Unexpected access to moved-from router.");
	}
	auto routing_result = (*m_router)(*mapped_topology);
	LOG4CXX_TRACE(m_logger, "Routed network in " << network_routing_timer.print() << ".");

	hate::Timer network_graph_timer;
	RoutingRewrite routing_rewrite(std::move(routing_result), mapped_topology);
	routing_rewrite();
	LOG4CXX_TRACE(
	    m_logger, "Constructed mapped topology in " << network_graph_timer.print() << ".");

	LOG4CXX_TRACE(m_logger, "Found: " << *mapped_topology << ".");

	hate::Timer valid_timer;
	if (!mapped_topology->valid()) {
		throw std::runtime_error("Found mapping invalid.");
	}

	// check that connectum of hardware network matches expected connectum of abstract network
	try {
		auto connectum_from_abstract_network =
		    generate_connectum_from_abstract_network(*mapped_topology);
		auto connectum_from_hardware_network =
		    generate_connectum_from_hardware_network(*mapped_topology);
		std::sort(connectum_from_abstract_network.begin(), connectum_from_abstract_network.end());
		std::sort(connectum_from_hardware_network.begin(), connectum_from_hardware_network.end());
		if (connectum_from_abstract_network != connectum_from_hardware_network) {
			std::vector<ConnectumConnection> missing_in_hardware_network;
			std::set_difference(
			    connectum_from_hardware_network.begin(), connectum_from_hardware_network.end(),
			    connectum_from_abstract_network.begin(), connectum_from_abstract_network.end(),
			    std::back_inserter(missing_in_hardware_network));
			std::vector<ConnectumConnection> missing_in_abstract_network;
			std::set_difference(
			    connectum_from_abstract_network.begin(), connectum_from_abstract_network.end(),
			    connectum_from_hardware_network.begin(), connectum_from_hardware_network.end(),
			    std::back_inserter(missing_in_abstract_network));
			if (missing_in_hardware_network.empty() && missing_in_abstract_network.empty()) {
				throw std::logic_error("Size difference in abstract and hardware connectum but no "
				                       "element difference found.");
			}
			LOG4CXX_ERROR(
			    m_logger, "Size mismatch between abstract and hardware connectum: abstract("
			                  << connectum_from_abstract_network.size() << "), hardware("
			                  << connectum_from_hardware_network.size() << ").");
			if (!missing_in_hardware_network.empty()) {
				std::stringstream ss;
				hate::IndentingOstream iss(ss);
				iss << "Abstract network connectum missing in hardware network:\n";
				iss << hate::Indentation("\t\t");
				iss << hate::join(missing_in_hardware_network, "\n");
				LOG4CXX_ERROR(m_logger, ss.str());
			}
			if (!missing_in_abstract_network.empty()) {
				std::stringstream ss;
				hate::IndentingOstream iss(ss);
				iss << hate::Indentation("\t\t");
				iss << "Hardware network connectum missing in abstract network:\n";
				iss << hate::join(missing_in_abstract_network, "\n");
				LOG4CXX_ERROR(m_logger, ss.str());
			}
			throw std::runtime_error("Found mapping invalid.");
		}
	} catch (InvalidNetworkGraph const& error) {
		LOG4CXX_ERROR(
		    m_logger, "Error during generation of connectum to validate: " << error.what());
	}

	LOG4CXX_TRACE(m_logger, "Checked validity of found mapping in " << valid_timer.print() << ".");

	return std::move(*mapped_topology);
}

} // namespace grenade::vx::network::abstract
