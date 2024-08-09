#include <gtest/gtest.h>

#include "grenade/vx/network/exception.h"
#include "grenade/vx/network/external_source_population.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/routing/portfolio_router.h"
#include "lola/vx/v3/synapse.h"

#include "hate/timer.h"
#include <log4cxx/logger.h>

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

namespace {

auto const get_logical_neuron = [](AtomicNeuronOnDLS const& an) {
	return LogicalNeuronOnDLS(
	    LogicalNeuronCompartments(
	        {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	    an);
};

} // namespace

TEST(NetworkGraph_valid, synapse_exchange)
{
	NetworkBuilder builder;

	grenade::common::ExecutionInstanceID instance;

	auto pre_descriptor = builder.add(ExternalSourcePopulation(
	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()}));

	Population population({
	    Population::Neuron(
	        get_logical_neuron(AtomicNeuronOnDLS(Enum(0))),
	        {{CompartmentOnLogicalNeuron(),
	          Population::Neuron::Compartment(
	              Population::Neuron::Compartment::SpikeMaster(0, true),
	              {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}})}}),
	});

	auto post_descriptor = builder.add(population);

	Projection::Connections connections{
	    Projection::Connection(
	        {0, CompartmentOnLogicalNeuron()}, {0, CompartmentOnLogicalNeuron()},
	        Projection::Connection::Weight(1)),
	    Projection::Connection(
	        {1, CompartmentOnLogicalNeuron()}, {0, CompartmentOnLogicalNeuron()},
	        Projection::Connection::Weight(1)),
	};
	Projection projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), connections, pre_descriptor,
	    post_descriptor);

	auto const projection_descriptor = builder.add(projection);

	auto network = builder.done();
	auto routing = routing::PortfolioRouter()(network);

	// expect error when we exchange the synapses of different sources to the same target in the
	// routing result
	auto connection_0 =
	    routing.execution_instances.at(instance).connections.at(projection_descriptor).at(0);
	auto connection_1 =
	    routing.execution_instances.at(instance).connections.at(projection_descriptor).at(1);
	routing.execution_instances.at(instance).connections.at(projection_descriptor).at(0) =
	    connection_1;
	routing.execution_instances.at(instance).connections.at(projection_descriptor).at(1) =
	    connection_0;

	EXPECT_THROW((build_network_graph(network, routing)), InvalidNetworkGraph);
}

TEST(NetworkGraph_valid, synapse_count)
{
	NetworkBuilder builder;

	grenade::common::ExecutionInstanceID instance;

	auto pre_descriptor =
	    builder.add(ExternalSourcePopulation({ExternalSourcePopulation::Neuron()}));

	Population population({
	    Population::Neuron(
	        get_logical_neuron(AtomicNeuronOnDLS(Enum(0))),
	        {{CompartmentOnLogicalNeuron(),
	          Population::Neuron::Compartment(
	              Population::Neuron::Compartment::SpikeMaster(0, true),
	              {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}})}}),
	});

	auto post_descriptor = builder.add(population);

	Projection::Connections connections{
	    Projection::Connection(
	        {0, CompartmentOnLogicalNeuron()}, {0, CompartmentOnLogicalNeuron()},
	        Projection::Connection::Weight(2 * 63)),
	};
	Projection projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), connections, pre_descriptor,
	    post_descriptor);

	auto const projection_descriptor = builder.add(projection);

	auto network = builder.done();
	auto routing = routing::PortfolioRouter()(network);

	// expect error when we exchange the label of a synapse in the routing result
	lola::vx::v3::SynapseMatrix::Label wrong_label(42); // arbitrary choice
	assert(
	    routing.execution_instances.at(instance)
	        .connections.at(projection_descriptor)
	        .at(0)
	        .at(0)
	        .label != wrong_label);
	routing.execution_instances.at(instance)
	    .connections.at(projection_descriptor)
	    .at(0)
	    .at(0)
	    .label = wrong_label;

	EXPECT_THROW((build_network_graph(network, routing)), InvalidNetworkGraph);
}
