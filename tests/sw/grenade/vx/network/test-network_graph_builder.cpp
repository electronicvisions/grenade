#include <gtest/gtest.h>

#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/routing_builder.h"

#include "hate/timer.h"
#include <log4cxx/logger.h>

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(build_network_graph, EmptyProjection)
{
	NetworkBuilder builder;

	Population population({AtomicNeuronOnDLS(Enum(0))}, {true});

	auto descriptor = builder.add(population);

	// empty projection
	Projection projection(Projection::ReceptorType::excitatory, {}, descriptor, descriptor);

	auto const projection_descriptor = builder.add(projection);

	auto network = builder.done();
	auto const routing_result = grenade::vx::network::build_routing(network);

	EXPECT_TRUE(routing_result.connections.contains(projection_descriptor));

	[[maybe_unused]] auto const network_graph = build_network_graph(network, routing_result);
}

TEST(update_network_graph, ProjectionOnBothHemispheres)
{
	NetworkBuilder builder;

	Population population(
	    {AtomicNeuronOnDLS(NeuronColumnOnDLS(0), NeuronRowOnDLS::top),
	     AtomicNeuronOnDLS(NeuronColumnOnDLS(0), NeuronRowOnDLS::bottom)},
	    {true, true});

	auto const descriptor = builder.add(population);

	Projection projection(
	    Projection::ReceptorType::excitatory,
	    {{0, 0, grenade::vx::network::Projection::Connection::Weight(63)},
	     {0, 1, grenade::vx::network::Projection::Connection::Weight(63)},
	     {1, 0, grenade::vx::network::Projection::Connection::Weight(63)},
	     {1, 1, grenade::vx::network::Projection::Connection::Weight(63)}},
	    descriptor, descriptor);

	builder.add(projection);
	auto const network = builder.done();

	auto const routing_result = grenade::vx::network::build_routing(network);

	auto network_graph = build_network_graph(network, routing_result);

	// alter weights
	for (auto& connection : projection.connections) {
		connection.weight = grenade::vx::network::Projection::Connection::Weight(0);
	}
	builder.add(population);
	builder.add(projection);
	auto const new_network = builder.done();

	EXPECT_NO_THROW(grenade::vx::network::update_network_graph(network_graph, new_network));
}

/**
 * Measures time consumption of generation of a single-layer feed-forward network, where the layers'
 * granularity, i.e. number of populations for fixed number of neurons is sweeped.
 * This is an informational-only test, since there are no assertions for time consumption limits.
 */
TEST(build_network_graph, GranularitySweep)
{
	static log4cxx::Logger* logger =
	    log4cxx::Logger::getLogger("grenade.build_network_graph.GranularitySweep");
	for (size_t num_pops = 1; num_pops <= NeuronColumnOnDLS::size; num_pops *= 2) {
		auto const neuron_per_pop = NeuronColumnOnDLS::size / num_pops;

		hate::Timer network_timer;
		NetworkBuilder builder;
		std::vector<PopulationDescriptor> input_pops;
		std::vector<PopulationDescriptor> hw_pops;
		for (size_t i = 0; i < num_pops; ++i) {
			grenade::vx::network::ExternalPopulation population_external{neuron_per_pop};
			input_pops.push_back(builder.add(population_external));

			grenade::vx::network::Population::Neurons neurons;
			grenade::vx::network::Population::EnableRecordSpikes enable_record_spikes;
			for (size_t n = i * neuron_per_pop; n < (i + 1) * neuron_per_pop; ++n) {
				neurons.push_back(AtomicNeuronOnDLS(NeuronColumnOnDLS(n), NeuronRowOnDLS::top));
				enable_record_spikes.push_back(true);
			}
			grenade::vx::network::Population population_internal{
			    std::move(neurons), enable_record_spikes};
			hw_pops.push_back(builder.add(population_internal));
		}

		for (auto const& stim : input_pops) {
			for (auto const& hwpop : hw_pops) {
				grenade::vx::network::Projection::Connections projection_connections;
				for (size_t i = 0; i < neuron_per_pop; ++i) {
					for (size_t j = 0; j < neuron_per_pop; ++j) {
						projection_connections.push_back(
						    {i, j, grenade::vx::network::Projection::Connection::Weight(63)});
					}
				}
				grenade::vx::network::Projection projection{
				    grenade::vx::network::Projection::ReceptorType::excitatory,
				    std::move(projection_connections), stim, hwpop};
				builder.add(projection);
			}
		}
		auto const network = builder.done();
		auto const network_timer_print = network_timer.print();
		hate::Timer build_routing_timer;
		auto const routing_result = grenade::vx::network::build_routing(network);
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
