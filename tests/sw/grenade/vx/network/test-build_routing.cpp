#include <gtest/gtest.h>

#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/routing/portfolio_router.h"

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(build_routing, ProjectionOverlap)
{
	NetworkBuilder builder;

	Population population{Population::Neurons{
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(Enum(0))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}),
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(Enum(1))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})}};
	ExternalSourcePopulation external_population(
	    {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()});

	// no overlap
	auto descriptor = builder.add(population);

	Projection projection_1(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{0, CompartmentOnLogicalNeuron()},
	      {0, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()}},
	    descriptor, descriptor);
	Projection projection_2(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{1, CompartmentOnLogicalNeuron()},
	      {1, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()}},
	    descriptor, descriptor);

	builder.add(projection_1);
	builder.add(projection_2);

	auto network = builder.done();
	EXPECT_NO_THROW(routing::PortfolioRouter()(network));

	// same projection
	descriptor = builder.add(population);

	Projection projection_3(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{0, CompartmentOnLogicalNeuron()},
	      {0, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()}},
	    descriptor, descriptor);

	builder.add(projection_3);
	builder.add(projection_3);

	network = builder.done();
	EXPECT_NO_THROW(routing::PortfolioRouter()(network));

	// overlap
	descriptor = builder.add(population);

	Projection projection_4(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{0, CompartmentOnLogicalNeuron()},
	      {0, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()},
	     {{1, CompartmentOnLogicalNeuron()},
	      {1, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()}},
	    descriptor, descriptor);
	Projection projection_5(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{1, CompartmentOnLogicalNeuron()},
	      {1, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()}},
	    descriptor, descriptor);

	builder.add(projection_4);
	builder.add(projection_5);

	network = builder.done();
	EXPECT_NO_THROW(routing::PortfolioRouter()(network));

	// from external no overlap
	auto input_descriptor = builder.add(external_population);
	descriptor = builder.add(population);

	projection_1 = Projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{0, CompartmentOnLogicalNeuron()},
	      {0, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()}},
	    input_descriptor, descriptor);
	projection_2 = Projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{1, CompartmentOnLogicalNeuron()},
	      {1, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()}},
	    input_descriptor, descriptor);

	builder.add(projection_1);
	builder.add(projection_2);

	network = builder.done();
	EXPECT_NO_THROW(routing::PortfolioRouter()(network));

	// from external same projection
	input_descriptor = builder.add(external_population);
	descriptor = builder.add(population);

	projection_3 = Projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{0, CompartmentOnLogicalNeuron()},
	      {0, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()}},
	    input_descriptor, descriptor);

	builder.add(projection_3);
	builder.add(projection_3);

	network = builder.done();
	EXPECT_NO_THROW(routing::PortfolioRouter()(network));

	// from external overlap
	input_descriptor = builder.add(external_population);
	descriptor = builder.add(population);

	projection_4 = Projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{0, CompartmentOnLogicalNeuron()},
	      {0, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()},
	     {{1, CompartmentOnLogicalNeuron()},
	      {1, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()}},
	    input_descriptor, descriptor);
	projection_5 = Projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{1, CompartmentOnLogicalNeuron()},
	      {1, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()}},
	    input_descriptor, descriptor);

	builder.add(projection_4);
	builder.add(projection_5);

	network = builder.done();
	EXPECT_NO_THROW(routing::PortfolioRouter()(network));
}

TEST(build_routing, EmptyProjection)
{
	NetworkBuilder builder;

	Population population{Population::Neurons{Population::Neuron(
	    LogicalNeuronOnDLS(
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	        AtomicNeuronOnDLS(Enum(0))),
	    Population::Neuron::Compartments{
	        {CompartmentOnLogicalNeuron(),
	         Population::Neuron::Compartment{
	             Population::Neuron::Compartment::SpikeMaster(0, true),
	             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})}};

	auto descriptor = builder.add(population);

	// empty projection
	Projection projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), {}, descriptor, descriptor);

	auto const projection_descriptor = builder.add(projection);

	auto network = builder.done();
	auto const routing_result = routing::PortfolioRouter()(network);

	EXPECT_TRUE(routing_result.execution_instances.at(grenade::vx::common::ExecutionInstanceID())
	                .connections.contains(projection_descriptor));
}

TEST(build_routing, DenseInOrder)
{
	NetworkBuilder builder;

	Population population{Population::Neurons{
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(Enum(0))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}),
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(Enum(1))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})}};
	auto const descriptor = builder.add(population);

	Projection projection(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory),
	    {{{0, CompartmentOnLogicalNeuron()},
	      {0, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()},
	     {{0, CompartmentOnLogicalNeuron()},
	      {1, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()},
	     {{1, CompartmentOnLogicalNeuron()},
	      {0, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()},
	     {{1, CompartmentOnLogicalNeuron()},
	      {1, CompartmentOnLogicalNeuron()},
	      Projection::Connection::Weight()}},
	    descriptor, descriptor);
	auto const projection_descriptor = builder.add(projection);

	PlasticityRule plasticity_rule;
	plasticity_rule.projections.push_back(projection_descriptor);
	plasticity_rule.enable_requires_one_source_per_row_in_order = true;
	builder.add(plasticity_rule);

	auto network = builder.done();
	EXPECT_NO_THROW(routing::PortfolioRouter()(network));
}

TEST(build_routing, I100H64O3)
{
	NetworkBuilder builder;

	ExternalSourcePopulation population_input;
	population_input.neurons.resize(100);
	auto const population_input_descriptor = builder.add(population_input);

	Population population_hidden;
	for (size_t i = 0; i < 64; ++i) {
		population_hidden.neurons.push_back(Population::Neuron(
		    LogicalNeuronOnDLS(
		        LogicalNeuronCompartments(
		            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
		        AtomicNeuronOnDLS(Enum(i))),
		    Population::Neuron::Compartments{
		        {CompartmentOnLogicalNeuron(),
		         Population::Neuron::Compartment{
		             Population::Neuron::Compartment::SpikeMaster(0, false),
		             {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
		               Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}));
	}
	auto const population_hidden_descriptor = builder.add(population_hidden);

	Population population_output{Population::Neurons{
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(Enum(256))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(Enum(257))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}}),
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(Enum(258))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, true),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory),
	                   Receptor(Receptor::ID(), Receptor::Type::inhibitory)}}}}})}};
	auto const population_output_descriptor = builder.add(population_output);

	Projection projection_excitatory_ih(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), {}, population_input_descriptor,
	    population_hidden_descriptor);
	Projection projection_inhibitory_ih(
	    Receptor(Receptor::ID(), Receptor::Type::inhibitory), {}, population_input_descriptor,
	    population_hidden_descriptor);
	for (size_t i = 0; i < population_input.neurons.size(); ++i) {
		for (size_t h = 0; h < population_hidden.neurons.size(); ++h) {
			projection_excitatory_ih.connections.push_back(Projection::Connection{
			    {i, CompartmentOnLogicalNeuron()},
			    {h, CompartmentOnLogicalNeuron()},
			    Projection::Connection::Weight()});
			projection_inhibitory_ih.connections.push_back(Projection::Connection{
			    {i, CompartmentOnLogicalNeuron()},
			    {h, CompartmentOnLogicalNeuron()},
			    Projection::Connection::Weight()});
		}
	}
	builder.add(projection_excitatory_ih);
	builder.add(projection_inhibitory_ih);

	Projection projection_excitatory_ho(
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), {}, population_hidden_descriptor,
	    population_output_descriptor);
	Projection projection_inhibitory_ho(
	    Receptor(Receptor::ID(), Receptor::Type::inhibitory), {}, population_hidden_descriptor,
	    population_output_descriptor);
	for (size_t h = 0; h < population_hidden.neurons.size(); ++h) {
		for (size_t o = 0; o < population_output.neurons.size(); ++o) {
			projection_excitatory_ho.connections.push_back(Projection::Connection{
			    {h, CompartmentOnLogicalNeuron()},
			    {o, CompartmentOnLogicalNeuron()},
			    Projection::Connection::Weight()});
			projection_inhibitory_ho.connections.push_back(Projection::Connection{
			    {h, CompartmentOnLogicalNeuron()},
			    {o, CompartmentOnLogicalNeuron()},
			    Projection::Connection::Weight()});
		}
	}
	builder.add(projection_excitatory_ho);
	builder.add(projection_inhibitory_ho);

	auto network = builder.done();
	EXPECT_NO_THROW(routing::PortfolioRouter()(network));
}
