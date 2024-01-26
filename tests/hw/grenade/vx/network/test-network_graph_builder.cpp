#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/backend/run.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/network/extract_output.h"
#include "grenade/vx/network/generate_input.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/routing/portfolio_router.h"
#include "grenade/vx/network/run.h"
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
using namespace grenade::vx::network;

TEST(NetworkGraphBuilder, FeedForwardOneToOne)
{
	// Construct connection to HW
	auto chip_configs = get_chip_configs_bypass_excitatory();
	grenade::vx::execution::JITGraphExecutor executor;

	grenade::vx::common::ExecutionInstanceID instance;

	// build network
	NetworkBuilder network_builder;

	ExternalSourcePopulation population_external{256};
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
	auto const routing_result = routing::PortfolioRouter()(network);
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
	    {{grenade::vx::common::ExecutionInstanceID(), grenade::vx::common::Time(num * isi)}});

	// run graph with given inputs and return results
	auto const result_map = run(executor, network_graph, chip_configs, inputs);

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

	grenade::vx::common::ExecutionInstanceID instance;

	// build network
	NetworkBuilder network_builder;

	ExternalSourcePopulation population_external{256};
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
	auto const routing_result = routing::PortfolioRouter()(network);
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
	    {{grenade::vx::common::ExecutionInstanceID(), grenade::vx::common::Time(num * isi)}});

	// run graph with given inputs and return results
	auto const result_map = run(executor, network_graph, chip_configs, inputs);

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

	grenade::vx::common::ExecutionInstanceID instance;

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

		ExternalSourcePopulation population_external{1};
		auto const population_external_descriptor = network_builder.add(population_external);

		std::vector<PopulationOnNetwork> population_internal_descriptors;
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
			     Projection::Connection::Weight(SynapseMatrix::Weight::max)}};
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
		auto const routing_result = routing::PortfolioRouter()(network);
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
		    inputs.batch_size(),
		    {{grenade::vx::common::ExecutionInstanceID(), grenade::vx::common::Time(num * isi)}});

		// run graph with given inputs and return results
		auto const result_map = run(executor, network_graph, chip_configs, inputs);

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

/**
 * Feed-forward network across two execution instances.
 * The first instance gets external user input and projects onto on-chip neurons.
 * Their output is then forwarded to the second instance via the host computer but without user
 * interaction. There, another projection onto on-chip neurons is performed, their output is read
 * back and compared to the expectation.
 */
TEST(NetworkGraphBuilder, ExecutionInstanceChain)
{
	// Construct connection to HW
	auto chip_configs = get_chip_configs_bypass_excitatory();
	chip_configs[grenade::vx::common::ExecutionInstanceID(1)] =
	    chip_configs.at(grenade::vx::common::ExecutionInstanceID(0));

	grenade::vx::execution::JITGraphExecutor executor;

	// build network
	NetworkBuilder network_builder;

	ExternalSourcePopulation population_0{1};
	auto const population_0_descriptor =
	    network_builder.add(population_0, grenade::vx::common::ExecutionInstanceID(0));

	Population::Neurons neurons_1{Population::Neuron(
	    LogicalNeuronOnDLS(
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	        AtomicNeuronOnDLS()),
	    Population::Neuron::Compartments{
	        {CompartmentOnLogicalNeuron(),
	         Population::Neuron::Compartment{
	             Population::Neuron::Compartment::SpikeMaster(0, true),
	             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})};
	Population population_1{std::move(neurons_1)};
	auto const population_1_descriptor = network_builder.add(population_1);

	{
		Projection::Connections projection_connections;
		for (size_t i = 0; i < population_0.size; ++i) {
			projection_connections.push_back(
			    {{i, CompartmentOnLogicalNeuron()},
			     {i, CompartmentOnLogicalNeuron()},
			     Projection::Connection::Weight(63)});
		}
		Projection projection{
		    Receptor(Receptor::ID(), Receptor::Type::excitatory), std::move(projection_connections),
		    population_0_descriptor.toPopulationOnExecutionInstance(),
		    population_1_descriptor.toPopulationOnExecutionInstance()};
		network_builder.add(projection);
	}

	ExternalSourcePopulation population_2{2};
	auto const population_2_descriptor =
	    network_builder.add(population_2, grenade::vx::common::ExecutionInstanceID(1));

	{
		InterExecutionInstanceProjection::Connections projection_connections;
		for (size_t i = 0; i < population_0.size; ++i) {
			projection_connections.push_back(
			    {{i, CompartmentOnLogicalNeuron()}, {i, CompartmentOnLogicalNeuron()}});
		}
		InterExecutionInstanceProjection projection{
		    std::move(projection_connections), population_1_descriptor, population_2_descriptor};
		network_builder.add(projection);
	}

	Population::Neurons neurons_3{
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS()),
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
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})};
	Population population_3{std::move(neurons_3)};
	auto const population_3_descriptor =
	    network_builder.add(population_3, grenade::vx::common::ExecutionInstanceID(1));

	{
		Projection::Connections projection_connections;
		for (size_t i = 0; i < population_2.size; ++i) {
			projection_connections.push_back(
			    {{i, CompartmentOnLogicalNeuron()},
			     {i, CompartmentOnLogicalNeuron()},
			     Projection::Connection::Weight(63)});
		}
		Projection projection{
		    Receptor(Receptor::ID(), Receptor::Type::excitatory), std::move(projection_connections),
		    population_2_descriptor.toPopulationOnExecutionInstance(),
		    population_3_descriptor.toPopulationOnExecutionInstance()};
		network_builder.add(projection, grenade::vx::common::ExecutionInstanceID(1));
	}

	// build network graph
	auto const network = network_builder.done();
	auto const routing_result = routing::PortfolioRouter()(network);
	auto const network_graph = build_network_graph(network, routing_result);

	// generate input
	constexpr size_t num = 100;
	constexpr size_t isi = 1000;
	InputGenerator input_generator(network_graph, 1);

	{
		std::vector<std::vector<std::vector<grenade::vx::common::Time>>> spike_times_0(
		    population_0.size);
		for (size_t i = 0; i < population_0.size; ++i) {
			spike_times_0.at(i).resize(population_0.size);
			for (size_t j = 0; j < num; ++j) {
				spike_times_0.at(i).at(i).push_back(grenade::vx::common::Time(j * isi));
			}
		}
		input_generator.add(spike_times_0, population_0_descriptor);
	}
	{
		std::vector<std::vector<std::vector<grenade::vx::common::Time>>> spike_times_2(
		    population_0.size);
		for (size_t i = 0; i < population_0.size; ++i) {
			spike_times_2.at(i).resize(population_2.size);
			for (size_t j = 0; j < num; ++j) {
				spike_times_2.at(i).at(i).push_back(grenade::vx::common::Time(j * isi + (isi / 2)));
			}
		}
		input_generator.add(spike_times_2, population_2_descriptor);
	}
	auto inputs = input_generator.done();
	inputs.runtime.resize(
	    inputs.batch_size(),
	    {{grenade::vx::common::ExecutionInstanceID(0), grenade::vx::common::Time(num * isi)},
	     {grenade::vx::common::ExecutionInstanceID(1), grenade::vx::common::Time(num * isi)}});

	// run graph with given inputs and return results
	auto const result_map = run(executor, network_graph, chip_configs, inputs);

	auto const result = extract_neuron_spikes(result_map, network_graph);

	EXPECT_EQ(result.size(), inputs.batch_size());
	for (size_t i = 0; i < population_3.neurons.size(); ++i) {
		for (size_t j = 0; j < result.size(); ++j) {
			if (i == j) {
				EXPECT_TRUE(result.at(j).contains(std::tuple{
				    population_3_descriptor, static_cast<size_t>(i),
				    CompartmentOnLogicalNeuron()}));
				if (result.at(j).contains(std::tuple{
				        population_3_descriptor, static_cast<size_t>(i),
				        CompartmentOnLogicalNeuron()})) {
					auto const& spikes = result.at(i).at(std::tuple{
					    population_3_descriptor, static_cast<size_t>(i),
					    CompartmentOnLogicalNeuron()});
					if (i == 0) {
						EXPECT_GE(spikes.size(), num * 0.8 * 2);
						EXPECT_LE(spikes.size(), num * 1.2 * 2);
					} else {
						EXPECT_GE(spikes.size(), num * 0.8);
						EXPECT_LE(spikes.size(), num * 1.2);
					}
				}
			} else {
				EXPECT_FALSE(result.at(j).contains(std::tuple{
				    population_3_descriptor, static_cast<size_t>(i),
				    CompartmentOnLogicalNeuron()}));
			}
		}
	}
}
