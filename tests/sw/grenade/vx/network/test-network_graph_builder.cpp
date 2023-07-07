#include <gtest/gtest.h>

#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/routing/portfolio_router.h"

#include "hate/timer.h"
#include <log4cxx/logger.h>

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

auto const get_logical_neuron = [](AtomicNeuronOnDLS const& an) {
	return LogicalNeuronOnDLS(
	    LogicalNeuronCompartments(
	        {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	    an);
};

TEST(logical_network_build_network_graph, Multapses)
{
	NetworkBuilder builder;

	Population population({
	    Population::Neuron(
	        get_logical_neuron(AtomicNeuronOnDLS(Enum(0))),
	        {{CompartmentOnLogicalNeuron(),
	          Population::Neuron::Compartment(
	              Population::Neuron::Compartment::SpikeMaster(0, true),
	              {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}})}}),
	    Population::Neuron(
	        get_logical_neuron(AtomicNeuronOnDLS(Enum(1))),
	        {{CompartmentOnLogicalNeuron(),
	          Population::Neuron::Compartment(
	              Population::Neuron::Compartment::SpikeMaster(0, true),
	              {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}})}}),
	});

	auto descriptor = builder.add(population);

	constexpr size_t max_weight_multiplier = 4;
	Projection::Connections connections{
	    Projection::Connection(
	        {0, CompartmentOnLogicalNeuron()}, {0, CompartmentOnLogicalNeuron()},
	        Projection::Connection::Weight(12)),
	    Projection::Connection(
	        {0, CompartmentOnLogicalNeuron()}, {1, CompartmentOnLogicalNeuron()},
	        Projection::Connection::Weight(100)),
	    Projection::Connection(
	        {1, CompartmentOnLogicalNeuron()}, {0, CompartmentOnLogicalNeuron()},
	        Projection::Connection::Weight(54)),
	    Projection::Connection(
	        {1, CompartmentOnLogicalNeuron()}, {1, CompartmentOnLogicalNeuron()},
	        Projection::Connection::Weight(
	            lola::vx::v3::SynapseMatrix::Weight::max * max_weight_multiplier)),
	};
	Projection projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), connections, descriptor, descriptor);

	auto const projection_descriptor = builder.add(projection);

	auto network = builder.done();
	auto const routing = routing::PortfolioRouter()(network);
	auto const network_graph = build_network_graph(network, routing);

	EXPECT_EQ(
	    network_graph.get_graph_translation().projections.at(projection_descriptor).at(0).size(),
	    1);
	EXPECT_EQ(
	    network_graph.get_graph_translation().projections.at(projection_descriptor).at(1).size(),
	    2);
	EXPECT_EQ(
	    network_graph.get_graph_translation().projections.at(projection_descriptor).at(2).size(),
	    1);
	EXPECT_EQ(
	    network_graph.get_graph_translation().projections.at(projection_descriptor).at(3).size(),
	    max_weight_multiplier);
	{
		auto const& local_translation =
		    network_graph.get_graph_translation().projections.at(projection_descriptor).at(0);
		EXPECT_EQ(
		    connections.at(0).weight.value(),
		    std::get<grenade::vx::signal_flow::vertex::SynapseArrayViewSparse>(
		        network_graph.get_graph().get_vertex_property(local_translation.at(0).first))
		        .get_synapses()[local_translation.at(0).second]
		        .weight.value());
	}
	{
		auto const& local_translation =
		    network_graph.get_graph_translation().projections.at(projection_descriptor).at(1);
		EXPECT_EQ(
		    lola::vx::v3::SynapseMatrix::Weight::max,
		    std::get<grenade::vx::signal_flow::vertex::SynapseArrayViewSparse>(
		        network_graph.get_graph().get_vertex_property(local_translation.at(0).first))
		        .get_synapses()[local_translation.at(0).second]
		        .weight.value());
		EXPECT_EQ(
		    connections.at(1).weight - lola::vx::v3::SynapseMatrix::Weight::max,
		    std::get<grenade::vx::signal_flow::vertex::SynapseArrayViewSparse>(
		        network_graph.get_graph().get_vertex_property(local_translation.at(1).first))
		        .get_synapses()[local_translation.at(1).second]
		        .weight.value());
	}
	{
		auto const& local_translation =
		    network_graph.get_graph_translation().projections.at(projection_descriptor).at(2);
		EXPECT_EQ(
		    connections.at(2).weight.value(),
		    std::get<grenade::vx::signal_flow::vertex::SynapseArrayViewSparse>(
		        network_graph.get_graph().get_vertex_property(local_translation.at(0).first))
		        .get_synapses()[local_translation.at(0).second]
		        .weight.value());
	}
	{
		auto const& local_translation =
		    network_graph.get_graph_translation().projections.at(projection_descriptor).at(3);
		EXPECT_EQ(
		    lola::vx::v3::SynapseMatrix::Weight::max,
		    std::get<grenade::vx::signal_flow::vertex::SynapseArrayViewSparse>(
		        network_graph.get_graph().get_vertex_property(local_translation.at(0).first))
		        .get_synapses()[local_translation.at(0).second]
		        .weight.value());
		EXPECT_EQ(
		    lola::vx::v3::SynapseMatrix::Weight::max,
		    std::get<grenade::vx::signal_flow::vertex::SynapseArrayViewSparse>(
		        network_graph.get_graph().get_vertex_property(local_translation.at(1).first))
		        .get_synapses()[local_translation.at(1).second]
		        .weight.value());
		EXPECT_EQ(
		    lola::vx::v3::SynapseMatrix::Weight::max,
		    std::get<grenade::vx::signal_flow::vertex::SynapseArrayViewSparse>(
		        network_graph.get_graph().get_vertex_property(local_translation.at(2).first))
		        .get_synapses()[local_translation.at(2).second]
		        .weight.value());
		EXPECT_EQ(
		    lola::vx::v3::SynapseMatrix::Weight::max,
		    std::get<grenade::vx::signal_flow::vertex::SynapseArrayViewSparse>(
		        network_graph.get_graph().get_vertex_property(local_translation.at(3).first))
		        .get_synapses()[local_translation.at(3).second]
		        .weight.value());
	}
}

TEST(build_network_graph, EmptyProjection)
{
	NetworkBuilder builder;

	Population population({Population::Neuron(
	    get_logical_neuron(AtomicNeuronOnDLS(Enum(0))),
	    {{CompartmentOnLogicalNeuron(),
	      Population::Neuron::Compartment(
	          Population::Neuron::Compartment::SpikeMaster(0, true),
	          {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}})}})});
	auto descriptor = builder.add(population);

	// empty projection
	Projection projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), {}, descriptor, descriptor);
	auto const projection_descriptor = builder.add(projection);
	auto network = builder.done();
	auto const routing_result = routing::PortfolioRouter()(network);

	EXPECT_TRUE(routing_result.connections.contains(projection_descriptor));

	[[maybe_unused]] auto const network_graph = build_network_graph(network, routing_result);
}

TEST(update_network_graph, ProjectionOnBothHemispheres)
{
	NetworkBuilder builder;

	Population population(
	    {Population::Neuron(
	         get_logical_neuron(AtomicNeuronOnDLS(NeuronColumnOnDLS(0), NeuronRowOnDLS::top)),
	         {{CompartmentOnLogicalNeuron(),
	           Population::Neuron::Compartment(
	               Population::Neuron::Compartment::SpikeMaster(0, true),
	               {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}})}}),
	     Population::Neuron(
	         get_logical_neuron(AtomicNeuronOnDLS(NeuronColumnOnDLS(0), NeuronRowOnDLS::bottom)),
	         {{CompartmentOnLogicalNeuron(),
	           Population::Neuron::Compartment(
	               Population::Neuron::Compartment::SpikeMaster(0, true),
	               {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}})}})});
	auto const descriptor = builder.add(population);

	Projection projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{0, CompartmentOnLogicalNeuron()},
	      {0, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight(63)},
	     {{0, CompartmentOnLogicalNeuron()},
	      {1, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight(63)},
	     {{1, CompartmentOnLogicalNeuron()},
	      {0, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight(63)},
	     {{1, CompartmentOnLogicalNeuron()},
	      {1, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight(63)}},
	    descriptor, descriptor);

	builder.add(projection);
	auto const network = builder.done();

	auto const routing_result = routing::PortfolioRouter()(network);

	auto network_graph = build_network_graph(network, routing_result);

	// alter weights
	for (auto& connection : projection.connections) {
		connection.weight = Projection::Connection::Weight(0);
	}
	builder.add(population);
	builder.add(projection);
	auto const new_network = builder.done();

	EXPECT_NO_THROW(update_network_graph(network_graph, new_network));
}

/**
 * Measures time consumption of generation of a single-layer feed-forward network, where the layers'
 * granularity, i.e. number of populations for fixed number of neurons is sweeped.
 * This is an informational-only test, since there are no assertions for time consumption limits.
 */
TEST(build_network_graph, GranularitySweep)
{
	log4cxx::LoggerPtr logger =
	    log4cxx::Logger::getLogger("grenade.build_network_graph.GranularitySweep");
	for (size_t num_pops = 1; num_pops <= NeuronColumnOnDLS::size; num_pops *= 2) {
		auto const neuron_per_pop = NeuronColumnOnDLS::size / num_pops;

		hate::Timer network_timer;
		NetworkBuilder builder;
		std::vector<PopulationDescriptor> input_pops;
		std::vector<PopulationDescriptor> hw_pops;
		for (size_t i = 0; i < num_pops; ++i) {
			ExternalSourcePopulation population_external{neuron_per_pop};
			input_pops.push_back(builder.add(population_external));
			Population population_internal;
			for (size_t n = i * neuron_per_pop; n < (i + 1) * neuron_per_pop; ++n) {
				population_internal.neurons.push_back(Population::Neuron(
				    get_logical_neuron(
				        AtomicNeuronOnDLS(NeuronColumnOnDLS(n), NeuronRowOnDLS::top)),
				    {{CompartmentOnLogicalNeuron(),
				      Population::Neuron::Compartment(
				          Population::Neuron::Compartment::SpikeMaster(0, true),
				          {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}})}}));
			}
			hw_pops.push_back(builder.add(population_internal));
		}

		for (auto const& stim : input_pops) {
			for (auto const& hwpop : hw_pops) {
				Projection::Connections projection_connections;
				for (size_t i = 0; i < neuron_per_pop; ++i) {
					for (size_t j = 0; j < neuron_per_pop; ++j) {
						projection_connections.push_back(
						    {{i, CompartmentOnLogicalNeuron()},
						     {j, CompartmentOnLogicalNeuron()},
						     Projection::Connection::Weight(63)});
					}
				}
				Projection projection{
				    Receptor(Receptor::ID(), Receptor::Type::excitatory),
				    std::move(projection_connections), stim, hwpop};
				builder.add(projection);
			}
		}
		auto const network = builder.done();
		auto const network_timer_print = network_timer.print();
		hate::Timer build_routing_timer;
		auto const routing_result = routing::PortfolioRouter()(network);
		auto const build_routing_timer_print = build_routing_timer.print();
		hate::Timer build_network_graph_timer;
		auto network_graph = build_network_graph(network, routing_result);
		auto const build_network_graph_timer_print = build_network_graph_timer.print();
		LOG4CXX_INFO(
		    logger, "num_pops(" << num_pops << ") network(" << network_timer_print << ") routing("
		                        << build_routing_timer_print << ") network_graph("
		                        << build_network_graph_timer_print << ").");
	}
}

TEST(build_network_graph, ExecutionInstance)
{
	NetworkBuilder builder;

	ExternalSourcePopulation population_external{1};
	auto const population_descriptor_input = builder.add(population_external);

	Population population_internal;
	population_internal.neurons.push_back(Population::Neuron(
	    get_logical_neuron(AtomicNeuronOnDLS()),
	    {{CompartmentOnLogicalNeuron(),
	      Population::Neuron::Compartment(
	          Population::Neuron::Compartment::SpikeMaster(0, true),
	          {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}})}}));
	auto const population_descriptor_output = builder.add(population_internal);

	Projection::Connections projection_connections;
	projection_connections.push_back(
	    {{0, CompartmentOnLogicalNeuron()},
	     {0, CompartmentOnLogicalNeuron()},
	     Projection::Connection::Weight(63)});
	Projection projection{
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), std::move(projection_connections),
	    population_descriptor_input, population_descriptor_output};
	builder.add(projection);

	auto const network = builder.done();

	auto const routing_result = routing::PortfolioRouter()(network);

	grenade::vx::signal_flow::ExecutionInstance execution_instance(
	    grenade::vx::signal_flow::ExecutionIndex(1), DLSGlobal(1));
	auto network_graph = build_network_graph(network, routing_result, execution_instance);

	for (auto const vertex :
	     boost::make_iterator_range(boost::vertices(network_graph.get_graph().get_graph()))) {
		EXPECT_EQ(
		    network_graph.get_graph().get_execution_instance_map().left.at(
		        network_graph.get_graph().get_vertex_descriptor_map().left.at(vertex)),
		    execution_instance);
	}
}
