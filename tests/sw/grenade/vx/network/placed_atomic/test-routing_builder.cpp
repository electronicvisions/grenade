#include <gtest/gtest.h>

#include "grenade/vx/network/placed_atomic/network_builder.h"
#include "grenade/vx/network/placed_atomic/routing_builder.h"

using namespace grenade::vx::network::placed_atomic;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(build_routing, ProjectionOverlap)
{
	NetworkBuilder builder;

	Population population({AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1))}, {true, true});
	ExternalPopulation external_population(2);

	// no overlap
	auto descriptor = builder.add(population);

	Projection projection_1(
	    Projection::ReceptorType::excitatory, {{0, 0, Projection::Connection::Weight()}},
	    descriptor, descriptor);
	Projection projection_2(
	    Projection::ReceptorType::excitatory, {{1, 1, Projection::Connection::Weight()}},
	    descriptor, descriptor);

	builder.add(projection_1);
	builder.add(projection_2);

	auto network = builder.done();
	EXPECT_NO_THROW(grenade::vx::network::placed_atomic::build_routing(network));

	// same projection
	descriptor = builder.add(population);

	Projection projection_3(
	    Projection::ReceptorType::excitatory, {{0, 0, Projection::Connection::Weight()}},
	    descriptor, descriptor);

	builder.add(projection_3);
	builder.add(projection_3);

	network = builder.done();
	EXPECT_NO_THROW(grenade::vx::network::placed_atomic::build_routing(network));

	// overlap
	descriptor = builder.add(population);

	Projection projection_4(
	    Projection::ReceptorType::excitatory,
	    {{0, 0, Projection::Connection::Weight()}, {1, 1, Projection::Connection::Weight()}},
	    descriptor, descriptor);
	Projection projection_5(
	    Projection::ReceptorType::excitatory, {{1, 1, Projection::Connection::Weight()}},
	    descriptor, descriptor);

	builder.add(projection_4);
	builder.add(projection_5);

	network = builder.done();
	EXPECT_NO_THROW(grenade::vx::network::placed_atomic::build_routing(network));

	// from external no overlap
	auto input_descriptor = builder.add(external_population);
	descriptor = builder.add(population);

	projection_1 = Projection(
	    Projection::ReceptorType::excitatory, {{0, 0, Projection::Connection::Weight()}},
	    input_descriptor, descriptor);
	projection_2 = Projection(
	    Projection::ReceptorType::excitatory, {{1, 1, Projection::Connection::Weight()}},
	    input_descriptor, descriptor);

	builder.add(projection_1);
	builder.add(projection_2);

	network = builder.done();
	EXPECT_NO_THROW(grenade::vx::network::placed_atomic::build_routing(network));

	// from external same projection
	input_descriptor = builder.add(external_population);
	descriptor = builder.add(population);

	projection_3 = Projection(
	    Projection::ReceptorType::excitatory, {{0, 0, Projection::Connection::Weight()}},
	    input_descriptor, descriptor);

	builder.add(projection_3);
	builder.add(projection_3);

	network = builder.done();
	EXPECT_NO_THROW(grenade::vx::network::placed_atomic::build_routing(network));

	// from external overlap
	input_descriptor = builder.add(external_population);
	descriptor = builder.add(population);

	projection_4 = Projection(
	    Projection::ReceptorType::excitatory,
	    {{0, 0, Projection::Connection::Weight()}, {1, 1, Projection::Connection::Weight()}},
	    input_descriptor, descriptor);
	projection_5 = Projection(
	    Projection::ReceptorType::excitatory, {{1, 1, Projection::Connection::Weight()}},
	    input_descriptor, descriptor);

	builder.add(projection_4);
	builder.add(projection_5);

	network = builder.done();
	EXPECT_NO_THROW(grenade::vx::network::placed_atomic::build_routing(network));
}

TEST(build_routing, EmptyProjection)
{
	NetworkBuilder builder;

	Population population({AtomicNeuronOnDLS(Enum(0))}, {true});

	auto descriptor = builder.add(population);

	// empty projection
	Projection projection(Projection::ReceptorType::excitatory, {}, descriptor, descriptor);

	auto const projection_descriptor = builder.add(projection);

	auto network = builder.done();
	auto const routing_result = grenade::vx::network::placed_atomic::build_routing(network);

	EXPECT_TRUE(routing_result.connections.contains(projection_descriptor));
}

TEST(build_routing, DenseInOrder)
{
	NetworkBuilder builder;

	Population population({AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1))}, {true, true});
	auto const descriptor = builder.add(population);

	Projection projection(
	    Projection::ReceptorType::excitatory,
	    {{0, 0, Projection::Connection::Weight()},
	     {0, 1, Projection::Connection::Weight()},
	     {1, 0, Projection::Connection::Weight()},
	     {1, 1, Projection::Connection::Weight()}},
	    descriptor, descriptor);
	auto const projection_descriptor = builder.add(projection);

	PlasticityRule plasticity_rule;
	plasticity_rule.projections.push_back(projection_descriptor);
	plasticity_rule.enable_requires_one_source_per_row_in_order = true;
	builder.add(plasticity_rule);

	auto network = builder.done();
	EXPECT_NO_THROW(grenade::vx::network::placed_atomic::build_routing(network));
}

TEST(build_routing, I100H64O3)
{
	NetworkBuilder builder;

	ExternalPopulation population_input(100);
	auto const population_input_descriptor = builder.add(population_input);

	Population population_hidden;
	population_hidden.enable_record_spikes.resize(64, false);
	for (size_t i = 0; i < population_hidden.enable_record_spikes.size(); ++i) {
		population_hidden.neurons.push_back(AtomicNeuronOnDLS(Enum(i)));
	}
	auto const population_hidden_descriptor = builder.add(population_hidden);

	Population population_output(
	    {AtomicNeuronOnDLS(Enum(256)), AtomicNeuronOnDLS(Enum(257)), AtomicNeuronOnDLS(Enum(258))},
	    {true, true, true});
	auto const population_output_descriptor = builder.add(population_output);

	Projection projection_excitatory_ih(
	    Projection::ReceptorType::excitatory, {}, population_input_descriptor,
	    population_hidden_descriptor);
	Projection projection_inhibitory_ih(
	    Projection::ReceptorType::inhibitory, {}, population_input_descriptor,
	    population_hidden_descriptor);
	for (size_t i = 0; i < population_input.size; ++i) {
		for (size_t h = 0; h < population_hidden.neurons.size(); ++h) {
			projection_excitatory_ih.connections.push_back(
			    Projection::Connection{i, h, Projection::Connection::Weight()});
			projection_inhibitory_ih.connections.push_back(
			    Projection::Connection{i, h, Projection::Connection::Weight()});
		}
	}
	builder.add(projection_excitatory_ih);
	builder.add(projection_inhibitory_ih);

	Projection projection_excitatory_ho(
	    Projection::ReceptorType::excitatory, {}, population_hidden_descriptor,
	    population_output_descriptor);
	Projection projection_inhibitory_ho(
	    Projection::ReceptorType::inhibitory, {}, population_hidden_descriptor,
	    population_output_descriptor);
	for (size_t h = 0; h < population_hidden.neurons.size(); ++h) {
		for (size_t o = 0; o < population_output.neurons.size(); ++o) {
			projection_excitatory_ho.connections.push_back(
			    Projection::Connection{h, o, Projection::Connection::Weight()});
			projection_inhibitory_ho.connections.push_back(
			    Projection::Connection{h, o, Projection::Connection::Weight()});
		}
	}
	builder.add(projection_excitatory_ho);
	builder.add(projection_inhibitory_ho);

	auto network = builder.done();
	EXPECT_NO_THROW(grenade::vx::network::placed_atomic::build_routing(network));
}
