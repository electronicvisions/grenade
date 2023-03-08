#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/backend/run.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/placed_atomic/build_routing.h"
#include "grenade/vx/network/placed_atomic/network.h"
#include "grenade/vx/network/placed_atomic/network_builder.h"
#include "grenade/vx/network/placed_atomic/network_graph.h"
#include "grenade/vx/network/placed_atomic/network_graph_builder.h"
#include "grenade/vx/network/placed_atomic/population.h"
#include "grenade/vx/network/placed_atomic/projection.h"
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

TEST(NetworkGraphBuilder, FeedForwardOneToOne)
{
	// Construct connection to HW
	auto chip_configs = get_chip_configs_bypass_excitatory();
	grenade::vx::execution::JITGraphExecutor executor;

	grenade::vx::signal_flow::ExecutionInstance instance;

	// build network
	grenade::vx::network::placed_atomic::NetworkBuilder network_builder;

	grenade::vx::network::placed_atomic::ExternalPopulation population_external{256};
	auto const population_external_descriptor = network_builder.add(population_external);

	grenade::vx::network::placed_atomic::Population::Neurons neurons;
	grenade::vx::network::placed_atomic::Population::EnableRecordSpikes enable_record_spikes;
	for (auto const column : iter_all<NeuronColumnOnDLS>()) {
		neurons.push_back(AtomicNeuronOnDLS(column, NeuronRowOnDLS::top));
		enable_record_spikes.push_back(true);
	}
	std::mt19937 rng{std::random_device{}()};
	std::shuffle(neurons.begin(), neurons.end(), rng);
	grenade::vx::network::placed_atomic::Population population_internal{
	    std::move(neurons), enable_record_spikes};
	auto const population_internal_descriptor = network_builder.add(population_internal);

	grenade::vx::network::placed_atomic::Projection::Connections projection_connections;
	for (size_t i = 0; i < population_external.size; ++i) {
		projection_connections.push_back(
		    {i, i, grenade::vx::network::placed_atomic::Projection::Connection::Weight(63)});
	}
	grenade::vx::network::placed_atomic::Projection projection{
	    grenade::vx::network::placed_atomic::Projection::ReceptorType::excitatory,
	    std::move(projection_connections), population_external_descriptor,
	    population_internal_descriptor};
	network_builder.add(projection);

	auto const network = network_builder.done();

	// build network graph
	auto const routing_result = grenade::vx::network::placed_atomic::build_routing(network);
	auto const network_graph =
	    grenade::vx::network::placed_atomic::build_network_graph(network, routing_result);

	// generate input
	grenade::vx::signal_flow::IODataMap inputs;
	constexpr size_t num = 100;
	constexpr size_t isi = 1000;
	std::vector<grenade::vx::signal_flow::TimedSpikeToChipSequence> input_spikes(256);
	for (size_t i = 0; i < population_external.size; ++i) {
		for (size_t j = 0; j < num; ++j) {
			assert(network_graph.get_spike_labels().at(population_external_descriptor).at(i).at(0));
			grenade::vx::signal_flow::TimedSpikeToChip spike{
			    grenade::vx::common::Time(j * isi), SpikePack1ToChip(SpikePack1ToChip::labels_type{
			                                            *(network_graph.get_spike_labels()
			                                                  .at(population_external_descriptor)
			                                                  .at(i)
			                                                  .at(0))})};
			input_spikes.at(i).push_back(spike);
		}
		inputs.runtime.push_back(
		    {{grenade::vx::signal_flow::ExecutionInstance(),
		      grenade::vx::common::Time(num * isi)}});
	}
	assert(network_graph.get_event_input_vertex());
	inputs.data[*network_graph.get_event_input_vertex()] = std::move(input_spikes);

	// run graph with given inputs and return results
	auto const result_map =
	    grenade::vx::execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

	assert(network_graph.get_event_output_vertex());
	auto const result = std::get<std::vector<grenade::vx::signal_flow::TimedSpikeFromChipSequence>>(
	    result_map.data.at(*network_graph.get_event_output_vertex()));

	EXPECT_EQ(
	    result.size(), std::get<std::vector<grenade::vx::signal_flow::TimedSpikeToChipSequence>>(
	                       inputs.data.at(*network_graph.get_event_input_vertex()))
	                       .size());
	for (size_t i = 0; i < population_internal.neurons.size(); ++i) {
		auto const& spikes = result.at(i);
		EXPECT_GE(spikes.size(), num * 0.8);
		EXPECT_LE(spikes.size(), num * 1.2);
		// count wrong spikes
		size_t not_matching = 0;
		auto const expected_label =
		    network_graph.get_spike_labels().at(population_internal_descriptor).at(i).at(0);
		assert(expected_label);
		for (auto const spike : spikes) {
			if (spike.data != *expected_label) {
				not_matching++;
			}
		}
		EXPECT_LE(not_matching, num * 0.2);
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
	grenade::vx::network::placed_atomic::NetworkBuilder network_builder;

	grenade::vx::network::placed_atomic::ExternalPopulation population_external{256};
	auto const population_external_descriptor = network_builder.add(population_external);

	grenade::vx::network::placed_atomic::Population::Neurons neurons;
	grenade::vx::network::placed_atomic::Population::EnableRecordSpikes enable_record_spikes;
	for (auto const column : iter_all<NeuronColumnOnDLS>()) {
		neurons.push_back(AtomicNeuronOnDLS(column, NeuronRowOnDLS::top));
		enable_record_spikes.push_back(true);
	}
	std::mt19937 rng{std::random_device{}()};
	std::shuffle(neurons.begin(), neurons.end(), rng);
	grenade::vx::network::placed_atomic::Population population_internal{
	    std::move(neurons), enable_record_spikes};
	auto const population_internal_descriptor = network_builder.add(population_internal);

	grenade::vx::network::placed_atomic::Projection::Connections projection_connections;
	for (size_t i = 0; i < population_external.size; ++i) {
		for (size_t j = 0; j < population_internal.neurons.size(); ++j) {
			// only set diagonal to ensure no spike drops
			// while this effectively is a very inefficient on-to-one connectivity, the algorithm
			// is forced to generate an all-to-all mapping
			projection_connections.push_back(
			    {i, j,
			     grenade::vx::network::placed_atomic::Projection::Connection::Weight(
			         i == j ? 63 : 0)});
		}
	}
	grenade::vx::network::placed_atomic::Projection projection{
	    grenade::vx::network::placed_atomic::Projection::ReceptorType::excitatory,
	    std::move(projection_connections), population_external_descriptor,
	    population_internal_descriptor};
	network_builder.add(projection);

	auto const network = network_builder.done();

	// build network graph
	auto const routing_result = grenade::vx::network::placed_atomic::build_routing(network);
	auto const network_graph =
	    grenade::vx::network::placed_atomic::build_network_graph(network, routing_result);

	// generate input
	grenade::vx::signal_flow::IODataMap inputs;
	constexpr size_t num = 100;
	constexpr size_t isi = 1000;
	std::vector<grenade::vx::signal_flow::TimedSpikeToChipSequence> input_spikes(256);
	for (size_t i = 0; i < population_external.size; ++i) {
		for (size_t j = 0; j < num; ++j) {
			assert(network_graph.get_spike_labels().at(population_external_descriptor).at(i).at(0));
			grenade::vx::signal_flow::TimedSpikeToChip spike{
			    grenade::vx::common::Time(j * isi), SpikePack1ToChip(SpikePack1ToChip::labels_type{
			                                            *(network_graph.get_spike_labels()
			                                                  .at(population_external_descriptor)
			                                                  .at(i)
			                                                  .at(0))})};
			input_spikes.at(i).push_back(spike);
		}
		inputs.runtime.push_back(
		    {{grenade::vx::signal_flow::ExecutionInstance(),
		      grenade::vx::common::Time(num * isi)}});
	}
	assert(network_graph.get_event_input_vertex());
	inputs.data[*network_graph.get_event_input_vertex()] = std::move(input_spikes);

	// run graph with given inputs and return results
	auto const result_map =
	    grenade::vx::execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

	assert(network_graph.get_event_output_vertex());
	auto const result = std::get<std::vector<grenade::vx::signal_flow::TimedSpikeFromChipSequence>>(
	    result_map.data.at(*network_graph.get_event_output_vertex()));

	EXPECT_EQ(
	    result.size(), std::get<std::vector<grenade::vx::signal_flow::TimedSpikeToChipSequence>>(
	                       inputs.data.at(*network_graph.get_event_input_vertex()))
	                       .size());
	for (size_t j = 0; j < population_external.size; ++j) {
		for (size_t i = 0; i < population_internal.neurons.size(); ++i) {
			auto const& spikes = result.at(i);
			// count correct spikes
			size_t matching = 0;
			auto const expected_label =
			    network_graph.get_spike_labels().at(population_internal_descriptor).at(i).at(0);
			assert(expected_label);
			for (auto const& spike : spikes) {
				if (spike.data == *expected_label) {
					matching++;
				} else {
					LOG4CXX_INFO(
					    logger, spike.data << spike.data.get_neuron_backend_address_out()
					                       << spike.data.get_neuron_event_output());
				}
			}
			EXPECT_GE(matching, num * 0.8);
			EXPECT_LE(matching, num * 1.2);
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
	std::vector<AtomicNeuronOnDLS> all_neurons;
	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
		all_neurons.push_back(neuron);
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
		grenade::vx::network::placed_atomic::NetworkBuilder network_builder;

		grenade::vx::network::placed_atomic::ExternalPopulation population_external{1};
		auto const population_external_descriptor = network_builder.add(population_external);

		std::vector<grenade::vx::network::placed_atomic::PopulationDescriptor>
		    population_internal_descriptors;
		for (size_t n = 0; n < length; ++n) {
			grenade::vx::network::placed_atomic::Population::Neurons neurons{all_neurons.at(n)};
			grenade::vx::network::placed_atomic::Population population_internal{
			    std::move(neurons), {n == length - 1}};
			population_internal_descriptors.push_back(network_builder.add(population_internal));
		}

		for (size_t i = 0; i < population_internal_descriptors.size(); ++i) {
			grenade::vx::network::placed_atomic::Projection::Connections projection_connections{
			    {0, 0,
			     grenade::vx::network::placed_atomic::Projection::Connection::Weight(
			         grenade::vx::network::placed_atomic::Projection::Connection::Weight::max)}};
			if (i == 0) {
				grenade::vx::network::placed_atomic::Projection projection{
				    grenade::vx::network::placed_atomic::Projection::ReceptorType::excitatory,
				    std::move(projection_connections), population_external_descriptor,
				    population_internal_descriptors.at(0)};
				network_builder.add(projection);
			} else {
				grenade::vx::network::placed_atomic::Projection projection{
				    grenade::vx::network::placed_atomic::Projection::ReceptorType::excitatory,
				    std::move(projection_connections), population_internal_descriptors.at(i - 1),
				    population_internal_descriptors.at(i)};
				network_builder.add(projection);
			}
		}

		auto const network = network_builder.done();

		// build network graph
		auto const routing_result = grenade::vx::network::placed_atomic::build_routing(network);
		auto const network_graph =
		    grenade::vx::network::placed_atomic::build_network_graph(network, routing_result);

		// generate input
		grenade::vx::signal_flow::IODataMap inputs;
		constexpr size_t num = 100;
		constexpr size_t isi = 12500;
		std::vector<grenade::vx::signal_flow::TimedSpikeToChipSequence> input_spikes(1);
		for (size_t j = 0; j < num; ++j) {
			assert(network_graph.get_spike_labels().at(population_external_descriptor).at(0).at(0));
			grenade::vx::signal_flow::TimedSpikeToChip spike{
			    grenade::vx::common::Time(j * isi), SpikePack1ToChip(SpikePack1ToChip::labels_type{
			                                            *(network_graph.get_spike_labels()
			                                                  .at(population_external_descriptor)
			                                                  .at(0)
			                                                  .at(0))})};
			input_spikes.at(0).push_back(spike);
		}
		inputs.runtime.push_back(
		    {{grenade::vx::signal_flow::ExecutionInstance(),
		      grenade::vx::common::Time(num * isi)}});
		assert(network_graph.get_event_input_vertex());
		inputs.data[*network_graph.get_event_input_vertex()] = std::move(input_spikes);

		// run graph with given inputs and return results
		auto const result_map =
		    grenade::vx::execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

		assert(network_graph.get_event_output_vertex());
		auto const result =
		    std::get<std::vector<grenade::vx::signal_flow::TimedSpikeFromChipSequence>>(
		        result_map.data.at(*network_graph.get_event_output_vertex()));

		EXPECT_EQ(
		    result.size(),
		    std::get<std::vector<grenade::vx::signal_flow::TimedSpikeToChipSequence>>(
		        inputs.data.at(*network_graph.get_event_input_vertex()))
		        .size());
		auto const& spikes = result.at(0);
		// count correct spikes
		size_t matching = 0;
		auto const expected_label =
		    network_graph.get_spike_labels().at(population_internal_descriptors.back()).at(0).at(0);
		assert(expected_label);
		for (auto const& spike : spikes) {
			if (spike.data == *expected_label) {
				matching++;
			}
		}
		EXPECT_GE(matching, num * 0.8) << "length: " << length << " for seed: " << seed;
		EXPECT_LE(matching, num * 1.2) << "length: " << length << " for seed: " << seed;
	}
}
