#include <gtest/gtest.h>

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/edge.h"
#include "grenade/common/inter_topology_hyper_edge/fixture.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/multi_index_sequence_dimension_unit/receptor_on_compartment.h"
#include "grenade/common/population.h"
#include "grenade/common/projection.h"
#include "grenade/common/projection_connector/sequence.h"
#include "grenade/common/receptor_on_compartment.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/topology.h"
#include "grenade/common/topology_rewrite/add_linked_topology.h"
#include "grenade/common/topology_rewrite/execution_instance.h"
#include "grenade/common/topology_rewrite/identity_replacement.h"
#include "grenade/common/topology_rewrite/population.h"
#include "grenade/common/topology_rewrite/projection.h"
#include "grenade/common/topology_rewrite/recorder.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/network/abstract/calibration/fixture.h"
#include "grenade/vx/network/abstract/mapped_topology_rewrite/placement.h"
#include "grenade/vx/network/abstract/mapper/greedy.h"
#include "grenade/vx/network/abstract/placement/greedy_placer.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated_signed.h"
#include "grenade/vx/network/abstract/resource_estimator/population.h"
#include "grenade/vx/network/abstract/topology_rewrite/plasticity_rule.h"
#include "grenade/vx/network/abstract/topology_rewrite/uncalibrated_signed_synapse.h"
#include "grenade/vx/network/build_connection_routing.h"
#include "grenade/vx/network/routing/greedy/routing_constraints.h"


using namespace grenade::common;
using namespace grenade::vx::network;
using namespace grenade::vx::network::abstract;
using namespace grenade::vx::network::routing::greedy;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

namespace {

std::shared_ptr<LinkedTopology> get_placed_topology(std::shared_ptr<Topology> const& topology)
{
	auto mapped_topology = std::make_shared<LinkedTopology>(topology);
	assert(mapped_topology);

	{
		IdentityReplacementTopologyRewrite copy_rewrite(mapped_topology);
		copy_rewrite();
	}

	{
		PopulationResourceEstimator population_resource_estimator(*mapped_topology, {});
		PopulationResourceEstimator::Resource population_system_resources(
		    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS::size,
		    halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock::size, {2});

		ExecutionInstanceTopologyRewrite::SystemResources executor_system_resources;
		executor_system_resources.emplace(ConnectionOnExecutor(), population_system_resources);

		PopulationTopologyRewrite population_rewrite(
		    population_resource_estimator, executor_system_resources, mapped_topology);
		population_rewrite();

		RecorderTopologyRewrite recorder_rewrite(mapped_topology);
		recorder_rewrite();

		PlasticityRuleRewrite plasticity_rule_rewrite(mapped_topology);
		plasticity_rule_rewrite();

		ProjectionTopologyRewrite projection_rewrite(mapped_topology);
		projection_rewrite();

		ExecutionInstanceTopologyRewrite execution_instance_rewrite(
		    population_resource_estimator, executor_system_resources, mapped_topology);
		execution_instance_rewrite();
	}

	auto placement_result = abstract::GreedyPlacer()(*mapped_topology);

	{
		AddLinkedTopologyRewrite add_linked_topology(mapped_topology);
		add_linked_topology();
	}

	PlacementRewrite placement_rewrite(std::move(placement_result), mapped_topology);
	placement_rewrite();

	return mapped_topology;
}

} // namespace


TEST(RoutingConstraints, check)
{
	// in-degree on PADI-bus too large
	{
		auto topology = std::make_shared<grenade::common::Topology>();

		grenade::common::Population population{
		    abstract::UncalibratedNeuron{
		        abstract::UncalibratedNeuron::Compartments{
		            {grenade::common::CompartmentOnNeuron(),
		             abstract::UncalibratedNeuron::Compartment{
		                 abstract::UncalibratedNeuron::Compartment::SpikeMaster(0),
		                 {{
		                     {grenade::common::ReceptorOnCompartment(0),
		                      Receptor::Type::excitatory},
		                 }}}}},
		        LogicalNeuronCompartments(
		            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
		    grenade::common::CuboidMultiIndexSequence(
		        {1}, grenade::common::MultiIndex({0}),
		        {grenade::common::CellOnPopulationDimensionUnit()}),
		    abstract::UncalibratedNeuron::ParameterSpace(
		        1, {{grenade::common::CompartmentOnNeuron(), 1}}),
		    grenade::common::TimeDomainOnTopology()};

		auto const pop_descriptor = topology->add_vertex(population);

		grenade::common::Projection projection(
		    abstract::UncalibratedSynapse{},
		    abstract::UncalibratedSynapse::ParameterSpace{{abstract::UncalibratedSynapse::Weight(
		        63 * (SynapseDriverOnPADIBus::size * SynapseRowOnSynapseDriver::size + 1))}},
		    SequenceConnector{
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence({1, 1})},
		    grenade::common::TimeDomainOnTopology());

		auto projection_descriptor = topology->add_vertex(projection);

		topology->add_edge(
		    pop_descriptor, projection_descriptor,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence(
		            {1, 1}, grenade::common::MultiIndex({0, 0}),
		            {grenade::common::CellOnPopulationDimensionUnit(),
		             grenade::common::CompartmentOnNeuronDimensionUnit()}),
		        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
		topology->add_edge(
		    projection_descriptor, pop_descriptor,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence(
		            {1, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
		            {grenade::common::CellOnPopulationDimensionUnit(),
		             grenade::common::CompartmentOnNeuronDimensionUnit(),
		             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
		        0, 0));

		auto const mapped_topology = get_placed_topology(topology);

		std::vector<VertexOnTopology> partitioned_vertex_descriptors;
		for (auto const& partitioned_vertex_descriptor :
		     mapped_topology->get_reference().vertices()) {
			partitioned_vertex_descriptors.emplace_back(partitioned_vertex_descriptor);
		}

		auto const connection_routing_result =
		    build_connection_routing(*mapped_topology, partitioned_vertex_descriptors);
		RoutingConstraints constraints(
		    *mapped_topology, partitioned_vertex_descriptors, connection_routing_result);
		EXPECT_THROW(constraints.check(), std::runtime_error);
	}
	// too many synapse rows on PADI-bus
	{
		auto topology = std::make_shared<grenade::common::Topology>();

		grenade::common::Population pop_0{
		    abstract::UncalibratedNeuron{
		        abstract::UncalibratedNeuron::Compartments{
		            {grenade::common::CompartmentOnNeuron(),
		             abstract::UncalibratedNeuron::Compartment{
		                 abstract::UncalibratedNeuron::Compartment::SpikeMaster(0),
		                 {{
		                     {grenade::common::ReceptorOnCompartment(0),
		                      Receptor::Type::excitatory},
		                 }}}}},
		        LogicalNeuronCompartments(
		            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
		    grenade::common::CuboidMultiIndexSequence(
		        {1}, grenade::common::MultiIndex({0}),
		        {grenade::common::CellOnPopulationDimensionUnit()}),
		    abstract::UncalibratedNeuron::ParameterSpace(
		        1, {{grenade::common::CompartmentOnNeuron(), 1}}),
		    grenade::common::TimeDomainOnTopology()};

		grenade::common::Population pop_1{
		    abstract::UncalibratedNeuron{
		        abstract::UncalibratedNeuron::Compartments{
		            {grenade::common::CompartmentOnNeuron(),
		             abstract::UncalibratedNeuron::Compartment{
		                 abstract::UncalibratedNeuron::Compartment::SpikeMaster(0),
		                 {{
		                     {grenade::common::ReceptorOnCompartment(0),
		                      Receptor::Type::inhibitory},
		                 }}}}},
		        LogicalNeuronCompartments(
		            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
		    grenade::common::CuboidMultiIndexSequence(
		        {1}, grenade::common::MultiIndex({0}),
		        {grenade::common::CellOnPopulationDimensionUnit()}),
		    abstract::UncalibratedNeuron::ParameterSpace(
		        1, {{grenade::common::CompartmentOnNeuron(), 1}}),
		    grenade::common::TimeDomainOnTopology()};

		auto const pop_0_descriptor = topology->add_vertex(pop_0);
		auto const pop_1_descriptor = topology->add_vertex(pop_1);

		grenade::common::Projection projection_exc(
		    abstract::UncalibratedSynapse{},
		    abstract::UncalibratedSynapse::ParameterSpace{{abstract::UncalibratedSynapse::Weight(
		        63 * (SynapseDriverOnPADIBus::size * SynapseRowOnSynapseDriver::size))}},
		    SequenceConnector{
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence({1, 1})},
		    grenade::common::TimeDomainOnTopology());

		grenade::common::Projection projection_inh(
		    abstract::UncalibratedSynapse{},
		    abstract::UncalibratedSynapse::ParameterSpace{
		        {abstract::UncalibratedSynapse::Weight(63)}},
		    SequenceConnector{
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence({1, 1})},
		    grenade::common::TimeDomainOnTopology());

		auto projection_exc_descriptor = topology->add_vertex(projection_exc);
		auto projection_inh_descriptor = topology->add_vertex(projection_exc);

		topology->add_edge(
		    pop_0_descriptor, projection_exc_descriptor,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence(
		            {1, 1}, grenade::common::MultiIndex({0, 0}),
		            {grenade::common::CellOnPopulationDimensionUnit(),
		             grenade::common::CompartmentOnNeuronDimensionUnit()}),
		        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
		topology->add_edge(
		    projection_exc_descriptor, pop_0_descriptor,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence(
		            {1, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
		            {grenade::common::CellOnPopulationDimensionUnit(),
		             grenade::common::CompartmentOnNeuronDimensionUnit(),
		             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
		        0, 0));

		topology->add_edge(
		    pop_1_descriptor, projection_inh_descriptor,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence(
		            {1, 1}, grenade::common::MultiIndex({0, 0}),
		            {grenade::common::CellOnPopulationDimensionUnit(),
		             grenade::common::CompartmentOnNeuronDimensionUnit()}),
		        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
		topology->add_edge(
		    projection_inh_descriptor, pop_1_descriptor,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence(
		            {1, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
		            {grenade::common::CellOnPopulationDimensionUnit(),
		             grenade::common::CompartmentOnNeuronDimensionUnit(),
		             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
		        0, 0));

		auto const mapped_topology = get_placed_topology(topology);

		std::vector<VertexOnTopology> partitioned_vertex_descriptors;
		for (auto const& partitioned_vertex_descriptor :
		     mapped_topology->get_reference().vertices()) {
			partitioned_vertex_descriptors.emplace_back(partitioned_vertex_descriptor);
		}

		auto const connection_routing_result =
		    build_connection_routing(*mapped_topology, partitioned_vertex_descriptors);
		RoutingConstraints constraints(
		    *mapped_topology, partitioned_vertex_descriptors, connection_routing_result);
		EXPECT_THROW(constraints.check(), std::runtime_error);
	}
}

TEST(RoutingConstraints_InternalConnection, toPADIBusOnDLS)
{
	RoutingConstraints::InternalConnection connection{
	    AtomicNeuronOnDLS(Enum(53)),
	    std::tuple{
	        grenade::common::VertexOnTopology(), 0,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron()},
	    AtomicNeuronOnDLS(Enum(270)), std::pair{grenade::common::VertexOnTopology(), 0},
	    Receptor::Type::excitatory};
	EXPECT_EQ(
	    connection.toPADIBusOnDLS(),
	    PADIBusOnDLS(PADIBusOnPADIBusBlock(1), PADIBusBlockOnDLS::bottom));
}

TEST(RoutingConstraints_BackgroundConnection, toPADIBusOnDLS)
{
	RoutingConstraints::BackgroundConnection connection{
	    BackgroundSpikeSourceOnDLS(5),
	    std::tuple{
	        grenade::common::VertexOnTopology(), 0,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron()},
	    AtomicNeuronOnDLS(Enum(270)), std::pair{grenade::common::VertexOnTopology(), 0},
	    Receptor::Type::excitatory};
	EXPECT_EQ(
	    connection.toPADIBusOnDLS(),
	    PADIBusOnDLS(PADIBusOnPADIBusBlock(1), PADIBusBlockOnDLS::bottom));
}

TEST(RoutingConstraints_ExternalConnection, toPADIBusBlockOnDLS)
{
	RoutingConstraints::ExternalConnection connection{
	    std::tuple{
	        grenade::common::VertexOnTopology(), 0,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron()},
	    AtomicNeuronOnDLS(Enum(270)), std::pair{grenade::common::VertexOnTopology(), 0},
	    Receptor::Type::excitatory};
	EXPECT_EQ(connection.toPADIBusBlockOnDLS(), PADIBusBlockOnDLS::bottom);
}

TEST(RoutingConstraints, get_internal_connections)
{
	//	NetworkBuilder builder;
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(0))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(1))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < pop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	Projection proj(
	//	    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, pop_descriptor,
	//	    pop_descriptor);
	//	auto const proj_descriptor_0 = builder.add(proj);
	//	auto const proj_descriptor_1 = builder.add(proj);
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const internal_connections = constraints.get_internal_connections();
	//
	//	EXPECT_EQ(internal_connections.size(), 8);
	//	EXPECT_TRUE(constraints.get_background_connections().empty());
	//	EXPECT_TRUE(constraints.get_external_connections().empty());
	//
	//	std::vector<RoutingConstraints::InternalConnection> expectation{
	//	    {AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_0,
	// 0}, 	     Receptor::Type::inhibitory}, 	    {AtomicNeuronOnDLS(Enum(0)),
	// AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_0, 1}, Receptor::Type::inhibitory},
	// {AtomicNeuronOnDLS(Enum(1)), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_0, 2},
	// Receptor::Type::inhibitory}, 	    {AtomicNeuronOnDLS(Enum(1)), AtomicNeuronOnDLS(Enum(1)),
	// std::pair{proj_descriptor_0, 3}, 	     Receptor::Type::inhibitory},
	// {AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_1, 0},
	// Receptor::Type::inhibitory}, 	    {AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1)),
	// std::pair{proj_descriptor_1, 1}, 	     Receptor::Type::inhibitory},
	//	    {AtomicNeuronOnDLS(Enum(1)), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_1,
	// 2}, 	     Receptor::Type::inhibitory}, 	    {AtomicNeuronOnDLS(Enum(1)),
	// AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_1, 3}, Receptor::Type::inhibitory},
	//	};
	//	EXPECT_EQ(internal_connections, expectation);
}

TEST(RoutingConstraints, get_background_connections)
{
	//	NetworkBuilder builder;
	//	BackgroundSourcePopulation::Config bgconfig;
	//	bgconfig.enable_random = true;
	//	BackgroundSourcePopulation bgpop(
	//	    {{false}, {false}}, {{HemisphereOnDLS(0), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(0))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(1))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const bgpop_descriptor = builder.add(bgpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < bgpop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	Projection proj(
	//	    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, bgpop_descriptor,
	//	    pop_descriptor);
	//	auto const proj_descriptor_0 = builder.add(proj);
	//	auto const proj_descriptor_1 = builder.add(proj);
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const background_connections = constraints.get_background_connections();
	//
	//	EXPECT_EQ(background_connections.size(), 8);
	//	EXPECT_TRUE(constraints.get_external_connections().empty());
	//	EXPECT_TRUE(constraints.get_internal_connections().empty());
	//
	//	std::vector<RoutingConstraints::BackgroundConnection> expectation{
	//	    {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_0,
	// 0}, 	     Receptor::Type::inhibitory}, 	    {BackgroundSpikeSourceOnDLS(1),
	// AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_0, 1}, Receptor::Type::inhibitory},
	// {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_0, 2},
	// Receptor::Type::inhibitory}, 	    {BackgroundSpikeSourceOnDLS(1),
	// AtomicNeuronOnDLS(Enum(1)),
	// std::pair{proj_descriptor_0, 3}, 	     Receptor::Type::inhibitory},
	// {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_1, 0},
	// Receptor::Type::inhibitory}, 	    {BackgroundSpikeSourceOnDLS(1),
	// AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_1, 1}, Receptor::Type::inhibitory},
	//	    {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_1,
	// 2}, 	     Receptor::Type::inhibitory}, 	    {BackgroundSpikeSourceOnDLS(1),
	// AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_1, 3}, Receptor::Type::inhibitory},
	//	};
	//	EXPECT_EQ(background_connections, expectation);
}

TEST(RoutingConstraints, get_external_connections)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(0))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(1))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < extpop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	Projection proj(
	//	    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, extpop_descriptor,
	//	    pop_descriptor);
	//	auto const proj_descriptor_0 = builder.add(proj);
	//	auto const proj_descriptor_1 = builder.add(proj);
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const external_connections = constraints.get_external_connections();
	//
	//	EXPECT_EQ(external_connections.size(), 8);
	//	EXPECT_TRUE(constraints.get_background_connections().empty());
	//	EXPECT_TRUE(constraints.get_internal_connections().empty());
	//
	//	std::vector<RoutingConstraints::ExternalConnection> expectation{
	//	    {AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_0, 0},
	// Receptor::Type::inhibitory}, 	    {AtomicNeuronOnDLS(Enum(1)),
	// std::pair{proj_descriptor_0, 1}, Receptor::Type::inhibitory}, {AtomicNeuronOnDLS(Enum(0)),
	// std::pair{proj_descriptor_0, 2}, Receptor::Type::inhibitory}, {AtomicNeuronOnDLS(Enum(1)),
	// std::pair{proj_descriptor_0, 3}, Receptor::Type::inhibitory}, {AtomicNeuronOnDLS(Enum(0)),
	// std::pair{proj_descriptor_1, 0}, Receptor::Type::inhibitory}, {AtomicNeuronOnDLS(Enum(1)),
	// std::pair{proj_descriptor_1, 1}, Receptor::Type::inhibitory}, {AtomicNeuronOnDLS(Enum(0)),
	// std::pair{proj_descriptor_1, 2}, Receptor::Type::inhibitory}, {AtomicNeuronOnDLS(Enum(1)),
	// std::pair{proj_descriptor_1, 3}, Receptor::Type::inhibitory},
	//	};
	//	EXPECT_EQ(external_connections, expectation);
}

TEST(RoutingConstraints, get_external_connections_per_hemisphere)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(256))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(257))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < extpop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	Projection proj(
	//	    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, extpop_descriptor,
	//	    pop_descriptor);
	//	auto const proj_descriptor_0 = builder.add(proj);
	//	auto const proj_descriptor_1 = builder.add(proj);
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const external_connections = constraints.get_external_connections_per_hemisphere();
	//
	//	EXPECT_TRUE(external_connections[HemisphereOnDLS(0)].empty());
	//	EXPECT_EQ(external_connections[HemisphereOnDLS(1)].size(), 1);
	//	EXPECT_TRUE(external_connections[HemisphereOnDLS(1)].contains(Receptor::Type::inhibitory));
	//
	//	halco::common::typed_array<
	//	    std::map<Receptor::Type, std::vector<std::pair<ProjectionOnExecutionInstance, size_t>>>,
	//	    halco::hicann_dls::vx::v3::HemisphereOnDLS>
	//	    expectation;
	//	expectation[HemisphereOnDLS(1)][Receptor::Type::inhibitory] =
	//	    std::vector<std::pair<ProjectionOnExecutionInstance, size_t>>{
	//	        std::pair<ProjectionOnExecutionInstance, size_t>{
	//	            proj_descriptor_0.toProjectionOnExecutionInstance(), 0},
	//	        std::pair<ProjectionOnExecutionInstance, size_t>{
	//	            proj_descriptor_0.toProjectionOnExecutionInstance(), 1},
	//	        std::pair<ProjectionOnExecutionInstance, size_t>{
	//	            proj_descriptor_0.toProjectionOnExecutionInstance(), 2},
	//	        std::pair<ProjectionOnExecutionInstance, size_t>{
	//	            proj_descriptor_0.toProjectionOnExecutionInstance(), 3},
	//	        std::pair<ProjectionOnExecutionInstance, size_t>{
	//	            proj_descriptor_1.toProjectionOnExecutionInstance(), 0},
	//	        std::pair<ProjectionOnExecutionInstance, size_t>{
	//	            proj_descriptor_1.toProjectionOnExecutionInstance(), 1},
	//	        std::pair<ProjectionOnExecutionInstance, size_t>{
	//	            proj_descriptor_1.toProjectionOnExecutionInstance(), 2},
	//	        std::pair<ProjectionOnExecutionInstance, size_t>{
	//	            proj_descriptor_1.toProjectionOnExecutionInstance(), 3}};
	//	EXPECT_EQ(external_connections, expectation);
}

TEST(RoutingConstraints, get_external_sources_to_hemisphere)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(256))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(257))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < extpop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	Projection proj(
	//	    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, extpop_descriptor,
	//	    pop_descriptor);
	//	builder.add(proj);
	//	builder.add(proj);
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const external_sources = constraints.get_external_sources_to_hemisphere();
	//
	//	EXPECT_TRUE(external_sources[HemisphereOnDLS(0)].empty());
	//	EXPECT_EQ(external_sources[HemisphereOnDLS(1)].size(), 1);
	//	EXPECT_TRUE(external_sources[HemisphereOnDLS(1)].contains(Receptor::Type::inhibitory));
	//
	//	halco::common::typed_array<
	//	    std::map<Receptor::Type, std::set<std::pair<PopulationOnExecutionInstance, size_t>>>,
	//	    halco::hicann_dls::vx::v3::HemisphereOnDLS>
	//	    expectation;
	//	expectation[HemisphereOnDLS(1)][Receptor::Type::inhibitory] =
	//	    std::set<std::pair<PopulationOnExecutionInstance, size_t>>{
	//	        std::pair<PopulationOnExecutionInstance, size_t>{extpop_descriptor, 0},
	//	        std::pair<PopulationOnExecutionInstance, size_t>{extpop_descriptor, 1}};
	//	EXPECT_EQ(external_sources, expectation);
}

TEST(RoutingConstraints, get_neuron_in_degree)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	BackgroundSourcePopulation::Config bgconfig;
	//	bgconfig.enable_random = true;
	//	BackgroundSourcePopulation bgpop(
	//	    {{false}, {false}}, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(256))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(257))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const bgpop_descriptor = builder.add(bgpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < pop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections,
	// extpop_descriptor, 		    pop_descriptor); 		builder.add(proj); builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, bgpop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, pop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const neuron_in_degree = constraints.get_neuron_in_degree();
	//
	//	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
	//		if (std::find_if(pop.neurons.begin(), pop.neurons.end(), [neuron](auto const& nrn) {
	//			    return nrn.coordinate.get_atomic_neurons().at(0) == neuron;
	//		    }) != pop.neurons.end()) {
	//			EXPECT_EQ(neuron_in_degree[neuron], 12);
	//		} else {
	//			EXPECT_EQ(neuron_in_degree[neuron], 0);
	//		}
	//	}
}

TEST(RoutingConstraints, get_neuron_in_degree_per_padi_bus)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	BackgroundSourcePopulation::Config bgconfig;
	//	bgconfig.enable_random = true;
	//	BackgroundSourcePopulation bgpop(
	//	    {{false}, {false}}, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(256))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(257))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const bgpop_descriptor = builder.add(bgpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < pop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections,
	// extpop_descriptor, 		    pop_descriptor); 		builder.add(proj); builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, bgpop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, pop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const neuron_in_degree = constraints.get_neuron_in_degree_per_padi_bus();
	//
	//	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
	//		if (std::find_if(pop.neurons.begin(), pop.neurons.end(), [neuron](auto const& nrn) {
	//			    return nrn.coordinate.get_atomic_neurons().at(0) == neuron;
	//		    }) != pop.neurons.end()) {
	//			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(0)], 4);
	//			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(1)], 4);
	//			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(2)], 0);
	//			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(3)], 0);
	//		} else {
	//			for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
	//				EXPECT_EQ(neuron_in_degree[neuron][padi_bus], 0);
	//			}
	//		}
	//	}
}

TEST(RoutingConstraints, get_neuron_in_degree_per_receptor_type)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	BackgroundSourcePopulation::Config bgconfig;
	//	bgconfig.enable_random = true;
	//	BackgroundSourcePopulation bgpop(
	//	    {{false}, {false}}, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(256))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(257))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const bgpop_descriptor = builder.add(bgpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < pop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections,
	// extpop_descriptor, 		    pop_descriptor); 		builder.add(proj); builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::excitatory), connections, bgpop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, pop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const neuron_in_degree = constraints.get_neuron_in_degree_per_receptor_type();
	//
	//	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
	//		if (std::find_if(pop.neurons.begin(), pop.neurons.end(), [neuron](auto const& nrn) {
	//			    return nrn.coordinate.get_atomic_neurons().at(0) == neuron;
	//		    }) != pop.neurons.end()) {
	//			EXPECT_EQ(neuron_in_degree[neuron].at(Receptor::Type::excitatory), 4);
	//			EXPECT_EQ(neuron_in_degree[neuron].at(Receptor::Type::inhibitory), 8);
	//		} else {
	//			EXPECT_TRUE(neuron_in_degree[neuron].empty());
	//		}
	//	}
}

TEST(RoutingConstraints, get_neuron_in_degree_per_receptor_type_per_padi_bus)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	BackgroundSourcePopulation::Config bgconfig;
	//	bgconfig.enable_random = true;
	//	BackgroundSourcePopulation bgpop(
	//	    {{false, false}}, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(256))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(257))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const bgpop_descriptor = builder.add(bgpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < pop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections,
	// extpop_descriptor, 		    pop_descriptor); 		builder.add(proj); builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::excitatory), connections, bgpop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, pop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const neuron_in_degree =
	// constraints.get_neuron_in_degree_per_receptor_type_per_padi_bus();
	//
	//	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
	//		if (std::find_if(pop.neurons.begin(), pop.neurons.end(), [neuron](auto const& nrn) {
	//			    return nrn.coordinate.get_atomic_neurons().at(0) == neuron;
	//		    }) != pop.neurons.end()) {
	//			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(0)].size(), 1);
	//			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(1)].size(), 1);
	//			EXPECT_EQ(
	//			    neuron_in_degree[neuron][PADIBusOnPADIBusBlock(0)].at(Receptor::Type::inhibitory),
	//			    4);
	//			EXPECT_EQ(
	//			    neuron_in_degree[neuron][PADIBusOnPADIBusBlock(1)].at(Receptor::Type::excitatory),
	//			    4);
	//			EXPECT_TRUE(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(2)].empty());
	//			EXPECT_TRUE(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(3)].empty());
	//		} else {
	//			for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
	//				EXPECT_TRUE(neuron_in_degree[neuron][padi_bus].empty());
	//			}
	//		}
	//	}
}

TEST(RoutingConstraints, get_num_synapse_rows_per_padi_bus_per_receptor_type)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	BackgroundSourcePopulation::Config bgconfig;
	//	bgconfig.enable_random = true;
	//	BackgroundSourcePopulation bgpop(
	//	    {{false, false}}, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(0)}}, bgconfig);
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(256))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(257))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const bgpop_descriptor = builder.add(bgpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < pop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			if (i == 3 && j == 3) {
	//				continue;
	//			}
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections,
	// extpop_descriptor, 		    pop_descriptor); 		builder.add(proj); builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::excitatory), connections, bgpop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, pop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const num_synapse_rows =
	// constraints.get_num_synapse_rows_per_padi_bus_per_receptor_type();
	//
	//	EXPECT_EQ(
	//	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(0),
	// PADIBusBlockOnDLS::bottom)].size(), 	    2); 	EXPECT_EQ(
	//	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(0), PADIBusBlockOnDLS::bottom)].at(
	//	        Receptor::Type::inhibitory),
	//	    4);
	//	EXPECT_EQ(
	//	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(0), PADIBusBlockOnDLS::bottom)].at(
	//	        Receptor::Type::excitatory),
	//	    4);
	//	EXPECT_TRUE(num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(1),
	// PADIBusBlockOnDLS::bottom)] 	                .empty());
	//	EXPECT_TRUE(num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(2),
	// PADIBusBlockOnDLS::bottom)] 	                .empty());
	//	EXPECT_TRUE(num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(3),
	// PADIBusBlockOnDLS::bottom)] 	                .empty()); 	for (auto const padi_bus :
	// iter_all<PADIBusOnPADIBusBlock>()) {
	// EXPECT_TRUE(num_synapse_rows[PADIBusOnDLS(padi_bus, PADIBusBlockOnDLS::top)].empty());
	//	}
}

TEST(RoutingConstraints, get_num_synapse_rows_per_padi_bus)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	BackgroundSourcePopulation::Config bgconfig;
	//	bgconfig.enable_random = true;
	//	BackgroundSourcePopulation bgpop(
	//	    {{false, false}}, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(0)}}, bgconfig);
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(256))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(257))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const bgpop_descriptor = builder.add(bgpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < pop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			if (i == 3 && j == 3) {
	//				continue;
	//			}
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections,
	// extpop_descriptor, 		    pop_descriptor); 		builder.add(proj); builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::excitatory), connections, bgpop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, pop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const num_synapse_rows = constraints.get_num_synapse_rows_per_padi_bus();
	//
	//	EXPECT_EQ(
	//	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(0), PADIBusBlockOnDLS::bottom)], 8);
	//	EXPECT_EQ(
	//	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(1), PADIBusBlockOnDLS::bottom)], 0);
	//	EXPECT_EQ(
	//	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(2), PADIBusBlockOnDLS::bottom)], 0);
	//	EXPECT_EQ(
	//	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(3), PADIBusBlockOnDLS::bottom)], 0);
	//	for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
	//		EXPECT_EQ(num_synapse_rows[PADIBusOnDLS(padi_bus, PADIBusBlockOnDLS::top)], 0);
	//	}
}

TEST(RoutingConstraints, get_neurons_on_event_output)
{
	//	NetworkBuilder builder;
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(66))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(22))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(0))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})});
	//	builder.add(pop);
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const neurons_on_event_output = constraints.get_neurons_on_event_output();
	//
	//	EXPECT_EQ(neurons_on_event_output.size(), 2);
	//	EXPECT_EQ(neurons_on_event_output.at(NeuronEventOutputOnDLS(Enum(0))).size(), 1);
	//	EXPECT_EQ(
	//	    neurons_on_event_output.at(NeuronEventOutputOnDLS(Enum(0))).at(0),
	//	    AtomicNeuronOnDLS(Enum(22)));
	//	EXPECT_EQ(neurons_on_event_output.at(NeuronEventOutputOnDLS(Enum(2))).size(), 1);
	//	EXPECT_EQ(
	//	    neurons_on_event_output.at(NeuronEventOutputOnDLS(Enum(2))).at(0),
	//	    AtomicNeuronOnDLS(Enum(66)));
}

TEST(RoutingConstraints, get_neurons_on_padi_bus)
{
	//	NetworkBuilder builder;
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(66))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(22))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < pop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	Projection proj(
	//	    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, pop_descriptor,
	//	    pop_descriptor);
	//	builder.add(proj);
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const neurons_on_padi_bus = constraints.get_neurons_on_padi_bus();
	//
	//	EXPECT_EQ(neurons_on_padi_bus.size(), 2);
	//	EXPECT_EQ(neurons_on_padi_bus.at(PADIBusOnDLS(Enum(0))).size(), 1);
	//	EXPECT_TRUE(
	//	    neurons_on_padi_bus.at(PADIBusOnDLS(Enum(0))).contains(AtomicNeuronOnDLS(Enum(22))));
	//	EXPECT_EQ(neurons_on_padi_bus.at(PADIBusOnDLS(Enum(2))).size(), 1);
	//	EXPECT_TRUE(
	//	    neurons_on_padi_bus.at(PADIBusOnDLS(Enum(2))).contains(AtomicNeuronOnDLS(Enum(66))));
}

TEST(RoutingConstraints, get_num_background_sources_on_padi_bus)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	BackgroundSourcePopulation::Config bgconfig;
	//	bgconfig.enable_random = true;
	//	BackgroundSourcePopulation bgpop(
	//	    {{false, false}}, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(0)}}, bgconfig);
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(256))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(257))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const bgpop_descriptor = builder.add(bgpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < pop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			if (i == 3 && j == 3) {
	//				continue;
	//			}
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections,
	// extpop_descriptor, 		    pop_descriptor); 		builder.add(proj); builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::excitatory), connections, bgpop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, pop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const num_background_sources = constraints.get_num_background_sources_on_padi_bus();
	//
	//	for (auto const background_source : iter_all<BackgroundSpikeSourceOnDLS>()) {
	//		if (background_source == BackgroundSpikeSourceOnDLS(4)) {
	//			EXPECT_EQ(num_background_sources[background_source.toPADIBusOnDLS()], 2);
	//		} else {
	//			EXPECT_EQ(num_background_sources[background_source.toPADIBusOnDLS()], 0);
	//		}
	//	}
}

TEST(RoutingConstraints, get_neuron_event_outputs_on_padi_bus)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	BackgroundSourcePopulation::Config bgconfig;
	//	bgconfig.enable_random = true;
	//	BackgroundSourcePopulation bgpop(
	//	    {{false, false}}, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(0)}}, bgconfig);
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(256))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(257))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const bgpop_descriptor = builder.add(bgpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < pop.neurons.size(); ++i) {
	//		for (size_t j = 0; j < pop.neurons.size(); ++j) {
	//			if (i == 3 && j == 3) {
	//				continue;
	//			}
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections,
	// extpop_descriptor, 		    pop_descriptor); 		builder.add(proj); builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::excitatory), connections, bgpop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, pop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const neuron_event_outputs = constraints.get_neuron_event_outputs_on_padi_bus();
	//
	//	EXPECT_EQ(neuron_event_outputs.size(), 1);
	//	EXPECT_EQ(neuron_event_outputs.at(PADIBusOnDLS(Enum(4))).size(), 1);
	//	EXPECT_TRUE(
	//	    neuron_event_outputs.at(PADIBusOnDLS(Enum(4))).contains(NeuronEventOutputOnDLS(Enum(0))));
}

TEST(RoutingConstraints, get_padi_bus_constraints)
{
	//	NetworkBuilder builder;
	//	ExternalSourcePopulation extpop(
	//	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});
	//	BackgroundSourcePopulation::Config bgconfig;
	//	bgconfig.enable_random = true;
	//	BackgroundSourcePopulation bgpop(
	//	    {{false}, {false}}, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	//	Population pop(Population::Neurons{
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(256))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(257))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	//	    Population::Neuron(
	//	        LogicalNeuronOnDLS(
	//	            LogicalNeuronCompartments(
	//	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	//	            AtomicNeuronOnDLS(Enum(32))),
	//	        Population::Neuron::Compartments{
	//	            {CompartmentOnLogicalNeuron(),
	//	             Population::Neuron::Compartment{
	//	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	//	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	//	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})});
	//	auto const extpop_descriptor = builder.add(extpop);
	//	auto const bgpop_descriptor = builder.add(bgpop);
	//	auto const pop_descriptor = builder.add(pop);
	//
	//	Projection::Connections connections;
	//	for (size_t i = 0; i < 2; ++i) {
	//		for (size_t j = 0; j < 2; ++j) {
	//			if (i == 3 && j == 3) {
	//				continue;
	//			}
	//			connections.push_back(Projection::Connection(
	//			    {i, CompartmentOnLogicalNeuron()}, {j, CompartmentOnLogicalNeuron()},
	//			    Projection::Connection::Weight()));
	//		}
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections,
	// extpop_descriptor, 		    pop_descriptor); 		builder.add(proj); builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::excitatory), connections, bgpop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//	{
	//		Projection proj(
	//		    Receptor(Receptor::ID(), Receptor::Type::inhibitory), connections, pop_descriptor,
	//		    pop_descriptor);
	//		builder.add(proj);
	//		builder.add(proj);
	//	}
	//
	//	auto const network = builder.done();
	//
	//	assert(network);
	//	auto const connection_routing_result = build_connection_routing(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()));
	//	RoutingConstraints constraints(
	//	    network->execution_instances.at(grenade::common::ExecutionInstanceID()),
	//	    connection_routing_result);
	//
	//	auto const padi_bus_constraints = constraints.get_padi_bus_constraints();
	//
	//	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
	//		EXPECT_EQ(
	//		    padi_bus_constraints.at(padi_bus).num_background_spike_sources,
	//		    padi_bus == PADIBusOnDLS(Enum(5)) ? 2 : 0);
	//	}
	//	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
	//		EXPECT_EQ(
	//		    padi_bus_constraints.at(padi_bus).neuron_sources.size(),
	//		    padi_bus == PADIBusOnDLS(Enum(4)) ? 2 : 0);
	//	}
	//	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
	//		if (padi_bus == PADIBusOnDLS(Enum(1)) || padi_bus == PADIBusOnDLS(Enum(5))) {
	//			EXPECT_EQ(padi_bus_constraints.at(padi_bus).only_recorded_neurons.size(), 1);
	//		} else if (padi_bus == PADIBusOnDLS(Enum(0))) {
	//			EXPECT_EQ(padi_bus_constraints.at(padi_bus).only_recorded_neurons.size(), 2);
	//		} else {
	//			EXPECT_EQ(padi_bus_constraints.at(padi_bus).only_recorded_neurons.size(), 0);
	//		}
	//	}
	//	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
	//		EXPECT_EQ(
	//		    padi_bus_constraints.at(padi_bus).internal_connections.size(),
	//		    padi_bus == PADIBusOnDLS(Enum(4)) ? 8 : 0);
	//	}
	//	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
	//		EXPECT_EQ(
	//		    padi_bus_constraints.at(padi_bus).background_connections.size(),
	//		    padi_bus == PADIBusOnDLS(Enum(5)) ? 8 : 0);
	//	}
}
