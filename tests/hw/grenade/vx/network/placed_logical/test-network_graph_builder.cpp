#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/backend/run.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/placed_logical/build_routing.h"
#include "grenade/vx/network/placed_logical/extract_output.h"
#include "grenade/vx/network/placed_logical/generate_input.h"
#include "grenade/vx/network/placed_logical/network.h"
#include "grenade/vx/network/placed_logical/network_builder.h"
#include "grenade/vx/network/placed_logical/network_graph.h"
#include "grenade/vx/network/placed_logical/network_graph_builder.h"
#include "grenade/vx/network/placed_logical/population.h"
#include "grenade/vx/network/placed_logical/projection.h"
#include "grenade/vx/signal_flow/execution_instance.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/timer.h"
#include "helper.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/init_generator.h"
#include "stadls/vx/v3/playback_generator.h"
#include "stadls/vx/v3/run.h"
#include <random>
#include <gtest/gtest.h>
#include <log4cxx/logger.h>

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace stadls::vx::v3;
using namespace lola::vx::v3;
using namespace haldls::vx::v3;
using namespace grenade::vx::network::placed_logical;

TEST(NetworkGraphBuilder, FeedForwardOneToOne)
{
	// Construct connection to HW
	auto chip_configs = get_chip_configs_bypass_excitatory();
	grenade::vx::execution::JITGraphExecutor executor;

	grenade::vx::signal_flow::ExecutionInstance instance;

	// build network
	NetworkBuilder network_builder;

	ExternalPopulation population_external{256};
	auto const population_external_descriptor = network_builder.add(population_external);

	Population::Neurons neurons;
	for (auto const column : iter_all<NeuronColumnOnDLS>()) {
		neurons.push_back(Population::Neuron(
		    LogicalNeuronOnDLS(
		        LogicalNeuronCompartments(
		            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
		        AtomicNeuronOnDLS(column, NeuronRowOnDLS::top)),
		    Population::Neuron::Compartments{
		        {CompartmentOnLogicalNeuron(),
		         Population::Neuron::Compartment{
		             Population::Neuron::Compartment::SpikeMaster(0, true),
		             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}));
	}
	std::mt19937 rng{std::random_device{}()};
	std::shuffle(neurons.begin(), neurons.end(), rng);
	Population population_internal{std::move(neurons)};
	auto const population_internal_descriptor = network_builder.add(population_internal);

	Projection::Connections projection_connections;
	for (size_t i = 0; i < population_external.size; ++i) {
		projection_connections.push_back(
		    {{i, CompartmentOnLogicalNeuron()},
		     {i, CompartmentOnLogicalNeuron()},
		     Projection::Connection::Weight(63)});
	}
	Projection projection{
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), std::move(projection_connections),
	    population_external_descriptor, population_internal_descriptor};
	network_builder.add(projection);

	// build network graph
	auto const network = network_builder.done();
	auto const routing_result = build_routing(network);
	auto const network_graph = build_network_graph(network, routing_result);

	// generate input
	constexpr size_t num = 100;
	constexpr size_t isi = 1000;
	std::vector<std::vector<std::vector<grenade::vx::common::Time>>> input_spike_times(
	    population_external.size);
	for (size_t i = 0; i < population_external.size; ++i) {
		input_spike_times.at(i).resize(population_external.size);
		for (size_t j = 0; j < num; ++j) {
			input_spike_times.at(i).at(i).push_back(grenade::vx::common::Time(j * isi));
		}
	}
	InputGenerator input_generator(network_graph, input_spike_times.size());
	input_generator.add(input_spike_times, population_external_descriptor);
	auto inputs = input_generator.done();
	inputs.runtime.resize(
	    inputs.batch_size(),
	    {{grenade::vx::signal_flow::ExecutionInstance(), grenade::vx::common::Time(num * isi)}});

	// run graph with given inputs and return results
	auto const result_map =
	    grenade::vx::execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

	auto const result = extract_neuron_spikes(result_map, network_graph);

	EXPECT_EQ(result.size(), inputs.batch_size());
	for (size_t i = 0; i < population_internal.neurons.size(); ++i) {
		for (size_t j = 0; j < result.size(); ++j) {
			if (i == j) {
				EXPECT_TRUE(result.at(j).contains(std::tuple{
				    population_internal_descriptor, static_cast<size_t>(i),
				    CompartmentOnLogicalNeuron()}));
				if (result.at(j).contains(std::tuple{
				        population_internal_descriptor, static_cast<size_t>(i),
				        CompartmentOnLogicalNeuron()})) {
					auto const& spikes = result.at(i).at(std::tuple{
					    population_internal_descriptor, static_cast<size_t>(i),
					    CompartmentOnLogicalNeuron()});
					EXPECT_GE(spikes.size(), num * 0.8);
					EXPECT_LE(spikes.size(), num * 1.2);
				}
			} else {
				EXPECT_FALSE(result.at(j).contains(std::tuple{
				    population_internal_descriptor, static_cast<size_t>(i),
				    CompartmentOnLogicalNeuron()}));
			}
		}
	}
}

TEST(NetworkGraphBuilder, FeedForwardAllToAll)
{
	log4cxx::LoggerPtr logger =
	    log4cxx::Logger::getLogger("grenade.NetworkBuilderTest.FeedForwardAllToAll");
	// Construct connection to HW
	auto chip_configs = get_chip_configs_bypass_excitatory();
	grenade::vx::execution::JITGraphExecutor executor;

	grenade::vx::signal_flow::ExecutionInstance instance;

	// build network
	NetworkBuilder network_builder;

	ExternalPopulation population_external{256};
	auto const population_external_descriptor = network_builder.add(population_external);

	Population::Neurons neurons;
	for (auto const column : iter_all<NeuronColumnOnDLS>()) {
		neurons.push_back(Population::Neuron(
		    LogicalNeuronOnDLS(
		        LogicalNeuronCompartments(
		            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
		        AtomicNeuronOnDLS(column, NeuronRowOnDLS::top)),
		    Population::Neuron::Compartments{
		        {CompartmentOnLogicalNeuron(),
		         Population::Neuron::Compartment{
		             Population::Neuron::Compartment::SpikeMaster(0, true),
		             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}));
	}
	std::mt19937 rng{std::random_device{}()};
	std::shuffle(neurons.begin(), neurons.end(), rng);
	Population population_internal{std::move(neurons)};
	auto const population_internal_descriptor = network_builder.add(population_internal);

	Projection::Connections projection_connections;
	for (size_t i = 0; i < population_external.size; ++i) {
		for (size_t j = 0; j < population_internal.neurons.size(); ++j) {
			// only set diagonal to ensure no spike drops
			// while this effectively is a very inefficient on-to-one connectivity, the algorithm
			// is forced to generate an all-to-all mapping
			projection_connections.push_back(
			    {{i, CompartmentOnLogicalNeuron()},
			     {j, CompartmentOnLogicalNeuron()},
			     Projection::Connection::Weight(i == j ? 63 : 0)});
		}
	}
	Projection projection{
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), std::move(projection_connections),
	    population_external_descriptor, population_internal_descriptor};
	network_builder.add(projection);

	auto const network = network_builder.done();

	// build network graph
	auto const routing_result = build_routing(network);
	auto const network_graph = build_network_graph(network, routing_result);

	// generate input
	constexpr size_t num = 100;
	constexpr size_t isi = 1000;
	std::vector<std::vector<std::vector<grenade::vx::common::Time>>> input_spike_times(
	    population_external.size);
	for (size_t i = 0; i < population_external.size; ++i) {
		input_spike_times.at(i).resize(population_external.size);
		for (size_t j = 0; j < num; ++j) {
			input_spike_times.at(i).at(i).push_back(grenade::vx::common::Time(j * isi));
		}
	}
	InputGenerator input_generator(network_graph, input_spike_times.size());
	input_generator.add(input_spike_times, population_external_descriptor);
	auto inputs = input_generator.done();
	inputs.runtime.resize(
	    inputs.batch_size(),
	    {{grenade::vx::signal_flow::ExecutionInstance(), grenade::vx::common::Time(num * isi)}});

	// run graph with given inputs and return results
	auto const result_map =
	    grenade::vx::execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

	auto const result = extract_neuron_spikes(result_map, network_graph);

	EXPECT_EQ(result.size(), inputs.batch_size());
	for (size_t j = 0; j < population_external.size; ++j) {
		for (size_t i = 0; i < population_internal.neurons.size(); ++i) {
			if (i == j) {
				EXPECT_TRUE(result.at(j).contains(std::tuple{
				    population_internal_descriptor, static_cast<size_t>(i),
				    CompartmentOnLogicalNeuron()}));
				if (result.at(j).contains(std::tuple{
				        population_internal_descriptor, static_cast<size_t>(i),
				        CompartmentOnLogicalNeuron()})) {
					size_t matching = result.at(j)
					                      .at(std::tuple{
					                          population_internal_descriptor,
					                          static_cast<size_t>(i), CompartmentOnLogicalNeuron()})
					                      .size();
					EXPECT_GE(matching, num * 0.8);
					EXPECT_LE(matching, num * 1.2);
				}
			} else {
				EXPECT_FALSE(result.at(j).contains(std::tuple{
				    population_internal_descriptor, static_cast<size_t>(i),
				    CompartmentOnLogicalNeuron()}));
			}
		}
	}
}

TEST(NetworkGraphBuilder, SynfireChain)
{
	// Construct connection to HW
	auto chip_configs = get_chip_configs_bypass_excitatory();
	grenade::vx::execution::JITGraphExecutor executor;

	grenade::vx::signal_flow::ExecutionInstance instance;

	// construct shuffled list of neurons
	Population::Neurons all_neurons;
	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
		all_neurons.push_back(Population::Neuron(
		    LogicalNeuronOnDLS(
		        LogicalNeuronCompartments(
		            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
		        neuron),
		    Population::Neuron::Compartments{
		        {CompartmentOnLogicalNeuron(),
		         Population::Neuron::Compartment{
		             Population::Neuron::Compartment::SpikeMaster(0, true),
		             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}));
	}
	auto const seed = std::random_device{}();
	std::mt19937 rng{seed};
	std::shuffle(all_neurons.begin(), all_neurons.end(), rng);

	std::vector<size_t> all_lengths;
	for (size_t length = 1; length < all_neurons.size(); ++length) {
		all_lengths.push_back(length);
	}
	// build network
	// 10 synfire-chain lengths are tested
	constexpr static size_t num = 10;
	std::set<size_t> lengths;
	std::sample(
	    all_lengths.begin(), all_lengths.end(), std::inserter(lengths, lengths.begin()), num, rng);
	// always test minimal and maximal length
	assert(!all_lengths.empty());
	lengths.insert(all_lengths.front());
	lengths.insert(all_lengths.back());
	for (auto const& length : lengths) {
		NetworkBuilder network_builder;

		ExternalPopulation population_external{1};
		auto const population_external_descriptor = network_builder.add(population_external);

		std::vector<PopulationDescriptor> population_internal_descriptors;
		for (size_t n = 0; n < length; ++n) {
			Population::Neurons neurons{all_neurons.at(n)};
			for (size_t i = 0; i < neurons.size(); ++i) {
				neurons.at(i)
				    .compartments.at(CompartmentOnLogicalNeuron())
				    .spike_master->enable_record_spikes = n == length - 1;
			}
			Population population_internal{std::move(neurons)};
			population_internal_descriptors.push_back(network_builder.add(population_internal));
		}

		for (size_t i = 0; i < population_internal_descriptors.size(); ++i) {
			Projection::Connections projection_connections{
			    {{0, CompartmentOnLogicalNeuron()},
			     {0, CompartmentOnLogicalNeuron()},
			     Projection::Connection::Weight(Projection::Connection::Weight::max)}};
			if (i == 0) {
				Projection projection{
				    Receptor(Receptor::ID(), Receptor::Type::excitatory),
				    std::move(projection_connections), population_external_descriptor,
				    population_internal_descriptors.at(0)};
				network_builder.add(projection);
			} else {
				Projection projection{
				    Receptor(Receptor::ID(), Receptor::Type::excitatory),
				    std::move(projection_connections), population_internal_descriptors.at(i - 1),
				    population_internal_descriptors.at(i)};
				network_builder.add(projection);
			}
		}

		auto const network = network_builder.done();

		// build network graph
		auto const routing_result = build_routing(network);
		auto const network_graph = build_network_graph(network, routing_result);

		// generate input
		constexpr size_t num = 100;
		constexpr size_t isi = 12500;
		std::vector<std::vector<std::vector<grenade::vx::common::Time>>> input_spike_times(1);
		input_spike_times.at(0).resize(population_external.size);
		for (size_t j = 0; j < num; ++j) {
			input_spike_times.at(0).at(0).push_back(grenade::vx::common::Time(j * isi));
		}
		InputGenerator input_generator(network_graph, input_spike_times.size());
		input_generator.add(input_spike_times, population_external_descriptor);
		auto inputs = input_generator.done();
		inputs.runtime.resize(
		    inputs.batch_size(), {{grenade::vx::signal_flow::ExecutionInstance(),
		                           grenade::vx::common::Time(num * isi)}});

		// run graph with given inputs and return results
		auto const result_map =
		    grenade::vx::execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

		auto const result = extract_neuron_spikes(result_map, network_graph);

		EXPECT_EQ(result.size(), inputs.batch_size());
		EXPECT_TRUE(result.at(0).contains(std::tuple{
		    population_internal_descriptors.back(), static_cast<size_t>(0),
		    CompartmentOnLogicalNeuron()}));
		if (result.at(0).contains(std::tuple{
		        population_internal_descriptors.back(), static_cast<size_t>(0),
		        CompartmentOnLogicalNeuron()})) {
			size_t matching = result.at(0)
			                      .at(std::tuple{
			                          population_internal_descriptors.back(),
			                          static_cast<size_t>(0), CompartmentOnLogicalNeuron()})
			                      .size();
			EXPECT_GE(matching, num * 0.8) << "length: " << length << " for seed: " << seed;
			EXPECT_LE(matching, num * 1.2) << "length: " << length << " for seed: " << seed;
		}
	}
}
