#include <gtest/gtest.h>

#include "grenade/vx/network/placed_atomic/network.h"
#include "grenade/vx/network/placed_atomic/network_builder.h"
#include "grenade/vx/network/placed_atomic/population.h"
#include "grenade/vx/network/placed_atomic/projection.h"
#include "grenade/vx/network/placed_atomic/routing_constraints.h"

using namespace grenade::vx::network::placed_atomic;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(RoutingConstraints, check)
{
	// in-degree too large for neuron
	{
		NetworkBuilder builder;
		Population pop({AtomicNeuronOnDLS()}, {false});
		auto const pop_descriptor = builder.add(pop);

		for (size_t i = 0; i < SynapseRowOnSynram::size + 1; ++i) {
			Projection proj(
			    Projection::ReceptorType::excitatory, Projection::Connections(1), pop_descriptor,
			    pop_descriptor);
			builder.add(proj);
		}

		auto const network = builder.done();

		assert(network);
		RoutingConstraints constraints(*network);
		EXPECT_THROW(constraints.check(), std::runtime_error);
	}
	// in-degree on PADI-bus too large
	{
		NetworkBuilder builder;
		Population pop({AtomicNeuronOnDLS()}, {false});
		auto const pop_descriptor = builder.add(pop);

		for (size_t i = 0; i < SynapseDriverOnPADIBus::size * SynapseRowOnSynapseDriver::size + 1;
		     ++i) {
			Projection proj(
			    Projection::ReceptorType::excitatory, Projection::Connections(1), pop_descriptor,
			    pop_descriptor);
			builder.add(proj);
		}

		auto const network = builder.done();

		assert(network);
		RoutingConstraints constraints(*network);
		EXPECT_THROW(constraints.check(), std::runtime_error);
	}
	// too many synapse rows on PADI-bus
	{
		NetworkBuilder builder;
		Population pop_0({AtomicNeuronOnDLS(Enum(0))}, {false});
		auto const pop_0_descriptor = builder.add(pop_0);
		Population pop_1({AtomicNeuronOnDLS(Enum(1))}, {false});
		auto const pop_1_descriptor = builder.add(pop_1);

		for (size_t i = 0; i < SynapseDriverOnPADIBus::size * SynapseRowOnSynapseDriver::size;
		     ++i) {
			Projection proj(
			    Projection::ReceptorType::excitatory, Projection::Connections(1), pop_0_descriptor,
			    pop_0_descriptor);
			builder.add(proj);
		}

		Projection proj(
		    Projection::ReceptorType::inhibitory, Projection::Connections(1), pop_1_descriptor,
		    pop_1_descriptor);
		builder.add(proj);

		auto const network = builder.done();

		assert(network);
		RoutingConstraints constraints(*network);

		EXPECT_THROW(constraints.check(), std::runtime_error);
	}
}

TEST(RoutingConstraints_InternalConnection, toPADIBusOnDLS)
{
	RoutingConstraints::InternalConnection connection{
	    AtomicNeuronOnDLS(Enum(53)), AtomicNeuronOnDLS(Enum(270)),
	    std::pair{ProjectionDescriptor(), 0}, Projection::ReceptorType::excitatory};
	EXPECT_EQ(
	    connection.toPADIBusOnDLS(),
	    PADIBusOnDLS(PADIBusOnPADIBusBlock(1), PADIBusBlockOnDLS::bottom));
}

TEST(RoutingConstraints_BackgroundConnection, toPADIBusOnDLS)
{
	RoutingConstraints::BackgroundConnection connection{
	    BackgroundSpikeSourceOnDLS(5), AtomicNeuronOnDLS(Enum(270)),
	    std::pair{ProjectionDescriptor(), 0}, Projection::ReceptorType::excitatory};
	EXPECT_EQ(
	    connection.toPADIBusOnDLS(),
	    PADIBusOnDLS(PADIBusOnPADIBusBlock(1), PADIBusBlockOnDLS::bottom));
}

TEST(RoutingConstraints_ExternalConnection, toPADIBusBlockOnDLS)
{
	RoutingConstraints::ExternalConnection connection{
	    AtomicNeuronOnDLS(Enum(270)), std::pair{ProjectionDescriptor(), 0},
	    Projection::ReceptorType::excitatory};
	EXPECT_EQ(connection.toPADIBusBlockOnDLS(), PADIBusBlockOnDLS::bottom);
}

TEST(RoutingConstraints, get_internal_connections)
{
	NetworkBuilder builder;
	Population pop({AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1))}, {false, false});
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < pop.neurons.size(); ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	Projection proj(
	    Projection::ReceptorType::inhibitory, connections, pop_descriptor, pop_descriptor);
	auto const proj_descriptor_0 = builder.add(proj);
	auto const proj_descriptor_1 = builder.add(proj);

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const internal_connections = constraints.get_internal_connections();

	EXPECT_EQ(internal_connections.size(), 8);
	EXPECT_TRUE(constraints.get_background_connections().empty());
	EXPECT_TRUE(constraints.get_external_connections().empty());

	std::vector<RoutingConstraints::InternalConnection> expectation{
	    {AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_0, 0},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_0, 1},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(1)), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_0, 2},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(1)), AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_0, 3},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_1, 0},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_1, 1},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(1)), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_1, 2},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(1)), AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_1, 3},
	     Projection::ReceptorType::inhibitory},
	};
	EXPECT_EQ(internal_connections, expectation);
}

TEST(RoutingConstraints, get_background_connections)
{
	NetworkBuilder builder;
	BackgroundSpikeSourcePopulation::Config bgconfig;
	bgconfig.enable_random = true;
	BackgroundSpikeSourcePopulation bgpop(
	    2, {{HemisphereOnDLS(0), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	Population pop({AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1))}, {false, false});
	auto const bgpop_descriptor = builder.add(bgpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < bgpop.size; ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	Projection proj(
	    Projection::ReceptorType::inhibitory, connections, bgpop_descriptor, pop_descriptor);
	auto const proj_descriptor_0 = builder.add(proj);
	auto const proj_descriptor_1 = builder.add(proj);

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const background_connections = constraints.get_background_connections();

	EXPECT_EQ(background_connections.size(), 8);
	EXPECT_TRUE(constraints.get_external_connections().empty());
	EXPECT_TRUE(constraints.get_internal_connections().empty());

	std::vector<RoutingConstraints::BackgroundConnection> expectation{
	    {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_0, 0},
	     Projection::ReceptorType::inhibitory},
	    {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_0, 1},
	     Projection::ReceptorType::inhibitory},
	    {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_0, 2},
	     Projection::ReceptorType::inhibitory},
	    {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_0, 3},
	     Projection::ReceptorType::inhibitory},
	    {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_1, 0},
	     Projection::ReceptorType::inhibitory},
	    {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_1, 1},
	     Projection::ReceptorType::inhibitory},
	    {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_1, 2},
	     Projection::ReceptorType::inhibitory},
	    {BackgroundSpikeSourceOnDLS(1), AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_1, 3},
	     Projection::ReceptorType::inhibitory},
	};
	EXPECT_EQ(background_connections, expectation);
}

TEST(RoutingConstraints, get_external_connections)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	Population pop({AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1))}, {false, false});
	auto const extpop_descriptor = builder.add(extpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < extpop.size; ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	Projection proj(
	    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
	auto const proj_descriptor_0 = builder.add(proj);
	auto const proj_descriptor_1 = builder.add(proj);

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const external_connections = constraints.get_external_connections();

	EXPECT_EQ(external_connections.size(), 8);
	EXPECT_TRUE(constraints.get_background_connections().empty());
	EXPECT_TRUE(constraints.get_internal_connections().empty());

	std::vector<RoutingConstraints::ExternalConnection> expectation{
	    {AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_0, 0},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_0, 1},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_0, 2},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_0, 3},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_1, 0},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_1, 1},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(0)), std::pair{proj_descriptor_1, 2},
	     Projection::ReceptorType::inhibitory},
	    {AtomicNeuronOnDLS(Enum(1)), std::pair{proj_descriptor_1, 3},
	     Projection::ReceptorType::inhibitory},
	};
	EXPECT_EQ(external_connections, expectation);
}

TEST(RoutingConstraints, get_external_connections_per_hemisphere)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	Population pop({AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257))}, {false, false});
	auto const extpop_descriptor = builder.add(extpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < extpop.size; ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	Projection proj(
	    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
	auto const proj_descriptor_0 = builder.add(proj);
	auto const proj_descriptor_1 = builder.add(proj);

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const external_connections = constraints.get_external_connections_per_hemisphere();

	EXPECT_TRUE(external_connections[HemisphereOnDLS(0)].empty());
	EXPECT_EQ(external_connections[HemisphereOnDLS(1)].size(), 1);
	EXPECT_TRUE(
	    external_connections[HemisphereOnDLS(1)].contains(Projection::ReceptorType::inhibitory));

	halco::common::typed_array<
	    std::map<Projection::ReceptorType, std::vector<std::pair<ProjectionDescriptor, size_t>>>,
	    halco::hicann_dls::vx::v3::HemisphereOnDLS>
	    expectation;
	expectation[HemisphereOnDLS(1)][Projection::ReceptorType::inhibitory] =
	    std::vector<std::pair<ProjectionDescriptor, size_t>>{
	        std::pair<ProjectionDescriptor, size_t>{proj_descriptor_0, 0},
	        std::pair<ProjectionDescriptor, size_t>{proj_descriptor_0, 1},
	        std::pair<ProjectionDescriptor, size_t>{proj_descriptor_0, 2},
	        std::pair<ProjectionDescriptor, size_t>{proj_descriptor_0, 3},
	        std::pair<ProjectionDescriptor, size_t>{proj_descriptor_1, 0},
	        std::pair<ProjectionDescriptor, size_t>{proj_descriptor_1, 1},
	        std::pair<ProjectionDescriptor, size_t>{proj_descriptor_1, 2},
	        std::pair<ProjectionDescriptor, size_t>{proj_descriptor_1, 3}};
	EXPECT_EQ(external_connections, expectation);
}

TEST(RoutingConstraints, get_external_sources_to_hemisphere)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	Population pop({AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257))}, {false, false});
	auto const extpop_descriptor = builder.add(extpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < extpop.size; ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	Projection proj(
	    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
	builder.add(proj);
	builder.add(proj);

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const external_sources = constraints.get_external_sources_to_hemisphere();

	EXPECT_TRUE(external_sources[HemisphereOnDLS(0)].empty());
	EXPECT_EQ(external_sources[HemisphereOnDLS(1)].size(), 1);
	EXPECT_TRUE(
	    external_sources[HemisphereOnDLS(1)].contains(Projection::ReceptorType::inhibitory));

	halco::common::typed_array<
	    std::map<Projection::ReceptorType, std::set<std::pair<PopulationDescriptor, size_t>>>,
	    halco::hicann_dls::vx::v3::HemisphereOnDLS>
	    expectation;
	expectation[HemisphereOnDLS(1)][Projection::ReceptorType::inhibitory] =
	    std::set<std::pair<PopulationDescriptor, size_t>>{
	        std::pair<PopulationDescriptor, size_t>{extpop_descriptor, 0},
	        std::pair<PopulationDescriptor, size_t>{extpop_descriptor, 1}};
	EXPECT_EQ(external_sources, expectation);
}

TEST(RoutingConstraints, get_neuron_in_degree)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	BackgroundSpikeSourcePopulation::Config bgconfig;
	bgconfig.enable_random = true;
	BackgroundSpikeSourcePopulation bgpop(
	    2, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	Population pop({AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257))}, {false, false});
	auto const extpop_descriptor = builder.add(extpop);
	auto const bgpop_descriptor = builder.add(bgpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < pop.neurons.size(); ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, bgpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, pop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const neuron_in_degree = constraints.get_neuron_in_degree();

	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
		if (std::find(pop.neurons.begin(), pop.neurons.end(), neuron) != pop.neurons.end()) {
			EXPECT_EQ(neuron_in_degree[neuron], 12);
		} else {
			EXPECT_EQ(neuron_in_degree[neuron], 0);
		}
	}
}

TEST(RoutingConstraints, get_neuron_in_degree_per_padi_bus)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	BackgroundSpikeSourcePopulation::Config bgconfig;
	bgconfig.enable_random = true;
	BackgroundSpikeSourcePopulation bgpop(
	    2, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	Population pop({AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257))}, {false, false});
	auto const extpop_descriptor = builder.add(extpop);
	auto const bgpop_descriptor = builder.add(bgpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < pop.neurons.size(); ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, bgpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, pop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const neuron_in_degree = constraints.get_neuron_in_degree_per_padi_bus();

	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
		if (std::find(pop.neurons.begin(), pop.neurons.end(), neuron) != pop.neurons.end()) {
			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(0)], 4);
			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(1)], 4);
			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(2)], 0);
			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(3)], 0);
		} else {
			for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
				EXPECT_EQ(neuron_in_degree[neuron][padi_bus], 0);
			}
		}
	}
}

TEST(RoutingConstraints, get_neuron_in_degree_per_receptor_type)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	BackgroundSpikeSourcePopulation::Config bgconfig;
	bgconfig.enable_random = true;
	BackgroundSpikeSourcePopulation bgpop(
	    2, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	Population pop({AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257))}, {false, false});
	auto const extpop_descriptor = builder.add(extpop);
	auto const bgpop_descriptor = builder.add(bgpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < pop.neurons.size(); ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::excitatory, connections, bgpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, pop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const neuron_in_degree = constraints.get_neuron_in_degree_per_receptor_type();

	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
		if (std::find(pop.neurons.begin(), pop.neurons.end(), neuron) != pop.neurons.end()) {
			EXPECT_EQ(neuron_in_degree[neuron].at(Projection::ReceptorType::excitatory), 4);
			EXPECT_EQ(neuron_in_degree[neuron].at(Projection::ReceptorType::inhibitory), 8);
		} else {
			EXPECT_TRUE(neuron_in_degree[neuron].empty());
		}
	}
}

TEST(RoutingConstraints, get_neuron_in_degree_per_receptor_type_per_padi_bus)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	BackgroundSpikeSourcePopulation::Config bgconfig;
	bgconfig.enable_random = true;
	BackgroundSpikeSourcePopulation bgpop(
	    2, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	Population pop({AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257))}, {false, false});
	auto const extpop_descriptor = builder.add(extpop);
	auto const bgpop_descriptor = builder.add(bgpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < pop.neurons.size(); ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::excitatory, connections, bgpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, pop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const neuron_in_degree = constraints.get_neuron_in_degree_per_receptor_type_per_padi_bus();

	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
		if (std::find(pop.neurons.begin(), pop.neurons.end(), neuron) != pop.neurons.end()) {
			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(0)].size(), 1);
			EXPECT_EQ(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(1)].size(), 1);
			EXPECT_EQ(
			    neuron_in_degree[neuron][PADIBusOnPADIBusBlock(0)].at(
			        Projection::ReceptorType::inhibitory),
			    4);
			EXPECT_EQ(
			    neuron_in_degree[neuron][PADIBusOnPADIBusBlock(1)].at(
			        Projection::ReceptorType::excitatory),
			    4);
			EXPECT_TRUE(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(2)].empty());
			EXPECT_TRUE(neuron_in_degree[neuron][PADIBusOnPADIBusBlock(3)].empty());
		} else {
			for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
				EXPECT_TRUE(neuron_in_degree[neuron][padi_bus].empty());
			}
		}
	}
}

TEST(RoutingConstraints, get_num_synapse_rows_per_padi_bus_per_receptor_type)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	BackgroundSpikeSourcePopulation::Config bgconfig;
	bgconfig.enable_random = true;
	BackgroundSpikeSourcePopulation bgpop(
	    2, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(0)}}, bgconfig);
	Population pop({AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257))}, {false, false});
	auto const extpop_descriptor = builder.add(extpop);
	auto const bgpop_descriptor = builder.add(bgpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < pop.neurons.size(); ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			if (i == 3 && j == 3) {
				continue;
			}
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::excitatory, connections, bgpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, pop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const num_synapse_rows = constraints.get_num_synapse_rows_per_padi_bus_per_receptor_type();

	EXPECT_EQ(
	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(0), PADIBusBlockOnDLS::bottom)].size(),
	    2);
	EXPECT_EQ(
	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(0), PADIBusBlockOnDLS::bottom)].at(
	        Projection::ReceptorType::inhibitory),
	    4);
	EXPECT_EQ(
	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(0), PADIBusBlockOnDLS::bottom)].at(
	        Projection::ReceptorType::excitatory),
	    4);
	EXPECT_TRUE(num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(1), PADIBusBlockOnDLS::bottom)]
	                .empty());
	EXPECT_TRUE(num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(2), PADIBusBlockOnDLS::bottom)]
	                .empty());
	EXPECT_TRUE(num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(3), PADIBusBlockOnDLS::bottom)]
	                .empty());
	for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
		EXPECT_TRUE(num_synapse_rows[PADIBusOnDLS(padi_bus, PADIBusBlockOnDLS::top)].empty());
	}
}

TEST(RoutingConstraints, get_num_synapse_rows_per_padi_bus)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	BackgroundSpikeSourcePopulation::Config bgconfig;
	bgconfig.enable_random = true;
	BackgroundSpikeSourcePopulation bgpop(
	    2, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(0)}}, bgconfig);
	Population pop({AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257))}, {false, false});
	auto const extpop_descriptor = builder.add(extpop);
	auto const bgpop_descriptor = builder.add(bgpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < pop.neurons.size(); ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			if (i == 3 && j == 3) {
				continue;
			}
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::excitatory, connections, bgpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, pop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const num_synapse_rows = constraints.get_num_synapse_rows_per_padi_bus();

	EXPECT_EQ(
	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(0), PADIBusBlockOnDLS::bottom)], 8);
	EXPECT_EQ(
	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(1), PADIBusBlockOnDLS::bottom)], 0);
	EXPECT_EQ(
	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(2), PADIBusBlockOnDLS::bottom)], 0);
	EXPECT_EQ(
	    num_synapse_rows[PADIBusOnDLS(PADIBusOnPADIBusBlock(3), PADIBusBlockOnDLS::bottom)], 0);
	for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
		EXPECT_EQ(num_synapse_rows[PADIBusOnDLS(padi_bus, PADIBusBlockOnDLS::top)], 0);
	}
}

TEST(RoutingConstraints, get_neurons_on_event_output)
{
	NetworkBuilder builder;
	Population pop(
	    {AtomicNeuronOnDLS(Enum(66)), AtomicNeuronOnDLS(Enum(22)), AtomicNeuronOnDLS(Enum(0))},
	    {true, true, false});
	builder.add(pop);

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const neurons_on_event_output = constraints.get_neurons_on_event_output();

	EXPECT_EQ(neurons_on_event_output.size(), 2);
	EXPECT_EQ(neurons_on_event_output.at(NeuronEventOutputOnDLS(Enum(0))).size(), 1);
	EXPECT_EQ(
	    neurons_on_event_output.at(NeuronEventOutputOnDLS(Enum(0))).at(0),
	    AtomicNeuronOnDLS(Enum(22)));
	EXPECT_EQ(neurons_on_event_output.at(NeuronEventOutputOnDLS(Enum(2))).size(), 1);
	EXPECT_EQ(
	    neurons_on_event_output.at(NeuronEventOutputOnDLS(Enum(2))).at(0),
	    AtomicNeuronOnDLS(Enum(66)));
}

TEST(RoutingConstraints, get_neurons_on_padi_bus)
{
	NetworkBuilder builder;
	Population pop({AtomicNeuronOnDLS(Enum(66)), AtomicNeuronOnDLS(Enum(22))}, {true, true});
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < pop.neurons.size(); ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	Projection proj(
	    Projection::ReceptorType::inhibitory, connections, pop_descriptor, pop_descriptor);
	builder.add(proj);

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const neurons_on_padi_bus = constraints.get_neurons_on_padi_bus();

	EXPECT_EQ(neurons_on_padi_bus.size(), 2);
	EXPECT_EQ(neurons_on_padi_bus.at(PADIBusOnDLS(Enum(0))).size(), 1);
	EXPECT_TRUE(
	    neurons_on_padi_bus.at(PADIBusOnDLS(Enum(0))).contains(AtomicNeuronOnDLS(Enum(22))));
	EXPECT_EQ(neurons_on_padi_bus.at(PADIBusOnDLS(Enum(2))).size(), 1);
	EXPECT_TRUE(
	    neurons_on_padi_bus.at(PADIBusOnDLS(Enum(2))).contains(AtomicNeuronOnDLS(Enum(66))));
}

TEST(RoutingConstraints, get_num_background_sources_on_padi_bus)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	BackgroundSpikeSourcePopulation::Config bgconfig;
	bgconfig.enable_random = true;
	BackgroundSpikeSourcePopulation bgpop(
	    2, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(0)}}, bgconfig);
	Population pop({AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257))}, {false, false});
	auto const extpop_descriptor = builder.add(extpop);
	auto const bgpop_descriptor = builder.add(bgpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < pop.neurons.size(); ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			if (i == 3 && j == 3) {
				continue;
			}
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::excitatory, connections, bgpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, pop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const num_background_sources = constraints.get_num_background_sources_on_padi_bus();

	for (auto const background_source : iter_all<BackgroundSpikeSourceOnDLS>()) {
		if (background_source == BackgroundSpikeSourceOnDLS(4)) {
			EXPECT_EQ(num_background_sources[background_source.toPADIBusOnDLS()], 2);
		} else {
			EXPECT_EQ(num_background_sources[background_source.toPADIBusOnDLS()], 0);
		}
	}
}

TEST(RoutingConstraints, get_neuron_event_outputs_on_padi_bus)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	BackgroundSpikeSourcePopulation::Config bgconfig;
	bgconfig.enable_random = true;
	BackgroundSpikeSourcePopulation bgpop(
	    2, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(0)}}, bgconfig);
	Population pop({AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257))}, {false, false});
	auto const extpop_descriptor = builder.add(extpop);
	auto const bgpop_descriptor = builder.add(bgpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < pop.neurons.size(); ++i) {
		for (size_t j = 0; j < pop.neurons.size(); ++j) {
			if (i == 3 && j == 3) {
				continue;
			}
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::excitatory, connections, bgpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, pop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const neuron_event_outputs = constraints.get_neuron_event_outputs_on_padi_bus();

	EXPECT_EQ(neuron_event_outputs.size(), 1);
	EXPECT_EQ(neuron_event_outputs.at(PADIBusOnDLS(Enum(4))).size(), 1);
	EXPECT_TRUE(
	    neuron_event_outputs.at(PADIBusOnDLS(Enum(4))).contains(NeuronEventOutputOnDLS(Enum(0))));
}

TEST(RoutingConstraints, get_padi_bus_constraints)
{
	NetworkBuilder builder;
	ExternalPopulation extpop(2);
	BackgroundSpikeSourcePopulation::Config bgconfig;
	bgconfig.enable_random = true;
	BackgroundSpikeSourcePopulation bgpop(
	    2, {{HemisphereOnDLS(1), PADIBusOnPADIBusBlock(1)}}, bgconfig);
	Population pop(
	    {AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257)), AtomicNeuronOnDLS(Enum(32))},
	    {true, true, true});
	auto const extpop_descriptor = builder.add(extpop);
	auto const bgpop_descriptor = builder.add(bgpop);
	auto const pop_descriptor = builder.add(pop);

	Projection::Connections connections;
	for (size_t i = 0; i < 2; ++i) {
		for (size_t j = 0; j < 2; ++j) {
			if (i == 3 && j == 3) {
				continue;
			}
			connections.push_back(Projection::Connection(i, j, Projection::Connection::Weight()));
		}
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, extpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::excitatory, connections, bgpop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}
	{
		Projection proj(
		    Projection::ReceptorType::inhibitory, connections, pop_descriptor, pop_descriptor);
		builder.add(proj);
		builder.add(proj);
	}

	auto const network = builder.done();

	assert(network);
	RoutingConstraints constraints(*network);

	auto const padi_bus_constraints = constraints.get_padi_bus_constraints();

	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		EXPECT_EQ(
		    padi_bus_constraints.at(padi_bus).num_background_spike_sources,
		    padi_bus == PADIBusOnDLS(Enum(5)) ? 2 : 0);
	}
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		EXPECT_EQ(
		    padi_bus_constraints.at(padi_bus).neuron_sources.size(),
		    padi_bus == PADIBusOnDLS(Enum(4)) ? 2 : 0);
	}
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		if (padi_bus == PADIBusOnDLS(Enum(1)) || padi_bus == PADIBusOnDLS(Enum(5))) {
			EXPECT_EQ(padi_bus_constraints.at(padi_bus).only_recorded_neurons.size(), 1);
		} else if (padi_bus == PADIBusOnDLS(Enum(0))) {
			EXPECT_EQ(padi_bus_constraints.at(padi_bus).only_recorded_neurons.size(), 2);
		} else {
			EXPECT_EQ(padi_bus_constraints.at(padi_bus).only_recorded_neurons.size(), 0);
		}
	}
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		EXPECT_EQ(
		    padi_bus_constraints.at(padi_bus).internal_connections.size(),
		    padi_bus == PADIBusOnDLS(Enum(4)) ? 8 : 0);
	}
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		EXPECT_EQ(
		    padi_bus_constraints.at(padi_bus).background_connections.size(),
		    padi_bus == PADIBusOnDLS(Enum(5)) ? 8 : 0);
	}
}
