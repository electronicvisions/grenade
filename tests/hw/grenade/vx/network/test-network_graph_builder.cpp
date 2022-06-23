#include "grenade/vx/backend/connection.h"
#include "grenade/vx/backend/run.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/routing_builder.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/timer.h"
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

inline std::pair<lola::vx::v3::Chip, grenade::vx::backend::Connection>
initialize_excitatory_bypass()
{
	std::unique_ptr<lola::vx::v3::Chip> chip = std::make_unique<lola::vx::v3::Chip>();
	// Initialize chip
	ExperimentInit init;
	for (auto const c : iter_all<CommonNeuronBackendConfigOnDLS>()) {
		chip->neuron_block.backends[c].set_enable_clocks(true);
	}
	for (auto const c : iter_all<ColumnCurrentQuadOnDLS>()) {
		for (auto const e : iter_all<EntryOnQuad>()) {
			auto s = init.column_current_quad_config[c].get_switch(e);
			s.set_enable_synaptic_current_excitatory(true);
			s.set_enable_synaptic_current_inhibitory(true);
			init.column_current_quad_config[c].set_switch(e, s);
		}
	}
	grenade::vx::backend::Connection connection(hxcomm::vx::get_connection_from_env(), init);
	stadls::vx::v3::PlaybackProgramBuilder builder;
	// enable excitatory bypass mode
	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
		auto& config = chip->neuron_block.atomic_neurons[neuron];
		config.event_routing.enable_digital = true;
		config.event_routing.analog_output =
		    lola::vx::v3::AtomicNeuron::EventRouting::AnalogOutputMode::normal;
		config.event_routing.enable_bypass_excitatory = true;
		config.threshold.enable = false;
	}
	for (auto const block : iter_all<CommonPADIBusConfigOnDLS>()) {
		auto& padi_config = chip->synapse_driver_blocks[block.toSynapseDriverBlockOnDLS()].padi_bus;
		auto dacen = padi_config.get_dacen_pulse_extension();
		for (auto const block : iter_all<PADIBusOnPADIBusBlock>()) {
			dacen[block] = CommonPADIBusConfig::DacenPulseExtension(15);
		}
		padi_config.set_dacen_pulse_extension(dacen);
	}
	for (auto const drv : iter_all<SynapseDriverOnDLS>()) {
		auto& config = chip->synapse_driver_blocks[drv.toSynapseDriverBlockOnDLS()]
		                   .synapse_drivers[drv.toSynapseDriverOnSynapseDriverBlock()];
		config.set_enable_receiver(true);
		config.set_enable_address_out(true);
	}
	for (auto const block : iter_all<SynapseBlockOnDLS>()) {
		chip->synapse_blocks[block].i_bias_dac.fill(CapMemCell::Value(1022));
	}
	auto program = builder.done();
	grenade::vx::backend::run(connection, program);
	return std::make_pair(*chip, std::move(connection));
}

TEST(NetworkGraphBuilder, FeedForwardOneToOne)
{
	// Construct connection to HW
	auto [chip_config, connection] = initialize_excitatory_bypass();
	grenade::vx::JITGraphExecutor executor;
	executor.acquire_connection(DLSGlobal(), std::move(connection));

	grenade::vx::coordinate::ExecutionInstance instance;

	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[grenade::vx::coordinate::ExecutionInstance()] = chip_config;

	// build network
	grenade::vx::network::NetworkBuilder network_builder;

	grenade::vx::network::ExternalPopulation population_external{256};
	auto const population_external_descriptor = network_builder.add(population_external);

	grenade::vx::network::Population::Neurons neurons;
	grenade::vx::network::Population::EnableRecordSpikes enable_record_spikes;
	for (auto const column : iter_all<NeuronColumnOnDLS>()) {
		neurons.push_back(AtomicNeuronOnDLS(column, NeuronRowOnDLS::top));
		enable_record_spikes.push_back(true);
	}
	std::mt19937 rng{std::random_device{}()};
	std::shuffle(neurons.begin(), neurons.end(), rng);
	grenade::vx::network::Population population_internal{std::move(neurons), enable_record_spikes};
	auto const population_internal_descriptor = network_builder.add(population_internal);

	grenade::vx::network::Projection::Connections projection_connections;
	for (size_t i = 0; i < population_external.size; ++i) {
		projection_connections.push_back(
		    {i, i, grenade::vx::network::Projection::Connection::Weight(63)});
	}
	grenade::vx::network::Projection projection{
	    grenade::vx::network::Projection::ReceptorType::excitatory,
	    std::move(projection_connections), population_external_descriptor,
	    population_internal_descriptor};
	network_builder.add(projection);

	auto const network = network_builder.done();

	// build network graph
	auto const routing_result = grenade::vx::network::build_routing(network);
	auto const network_graph = grenade::vx::network::build_network_graph(network, routing_result);

	// generate input
	grenade::vx::IODataMap inputs;
	constexpr size_t num = 100;
	constexpr size_t isi = 1000;
	std::vector<grenade::vx::TimedSpikeSequence> input_spikes(256);
	for (size_t i = 0; i < population_external.size; ++i) {
		for (size_t j = 0; j < num; ++j) {
			assert(network_graph.get_spike_labels().at(population_external_descriptor).at(i).at(0));
			grenade::vx::TimedSpike spike{Timer::Value(j * isi),
			                              SpikePack1ToChip(SpikePack1ToChip::labels_type{
			                                  *(network_graph.get_spike_labels()
			                                        .at(population_external_descriptor)
			                                        .at(i)
			                                        .at(0))})};
			input_spikes.at(i).push_back(spike);
		}
		inputs.runtime[grenade::vx::coordinate::ExecutionInstance()].push_back(
		    Timer::Value(num * isi));
	}
	assert(network_graph.get_event_input_vertex());
	inputs.data[*network_graph.get_event_input_vertex()] = std::move(input_spikes);

	// run graph with given inputs and return results
	auto const result_map =
	    grenade::vx::run(executor, network_graph.get_graph(), inputs, chip_configs);

	assert(network_graph.get_event_output_vertex());
	auto const result = std::get<std::vector<grenade::vx::TimedSpikeFromChipSequence>>(
	    result_map.data.at(*network_graph.get_event_output_vertex()));

	EXPECT_EQ(
	    result.size(), std::get<std::vector<grenade::vx::TimedSpikeSequence>>(
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
			if (spike.label != *expected_label) {
				not_matching++;
			}
		}
		EXPECT_LE(not_matching, num * 0.2);
	}
}

TEST(NetworkGraphBuilder, FeedForwardAllToAll)
{
	static log4cxx::Logger* logger =
	    log4cxx::Logger::getLogger("grenade.NetworkBuilderTest.FeedForwardAllToAll");
	// Construct connection to HW
	auto [chip_config, connection] = initialize_excitatory_bypass();
	grenade::vx::JITGraphExecutor executor;
	executor.acquire_connection(DLSGlobal(), std::move(connection));

	grenade::vx::coordinate::ExecutionInstance instance;

	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[grenade::vx::coordinate::ExecutionInstance()] = chip_config;

	// build network
	grenade::vx::network::NetworkBuilder network_builder;

	grenade::vx::network::ExternalPopulation population_external{256};
	auto const population_external_descriptor = network_builder.add(population_external);

	grenade::vx::network::Population::Neurons neurons;
	grenade::vx::network::Population::EnableRecordSpikes enable_record_spikes;
	for (auto const column : iter_all<NeuronColumnOnDLS>()) {
		neurons.push_back(AtomicNeuronOnDLS(column, NeuronRowOnDLS::top));
		enable_record_spikes.push_back(true);
	}
	std::mt19937 rng{std::random_device{}()};
	std::shuffle(neurons.begin(), neurons.end(), rng);
	grenade::vx::network::Population population_internal{std::move(neurons), enable_record_spikes};
	auto const population_internal_descriptor = network_builder.add(population_internal);

	grenade::vx::network::Projection::Connections projection_connections;
	for (size_t i = 0; i < population_external.size; ++i) {
		for (size_t j = 0; j < population_internal.neurons.size(); ++j) {
			// only set diagonal to ensure no spike drops
			// while this effectively is a very inefficient on-to-one connectivity, the algorithm
			// is forced to generate an all-to-all mapping
			projection_connections.push_back(
			    {i, j, grenade::vx::network::Projection::Connection::Weight(i == j ? 63 : 0)});
		}
	}
	grenade::vx::network::Projection projection{
	    grenade::vx::network::Projection::ReceptorType::excitatory,
	    std::move(projection_connections), population_external_descriptor,
	    population_internal_descriptor};
	network_builder.add(projection);

	auto const network = network_builder.done();

	// build network graph
	auto const routing_result = grenade::vx::network::build_routing(network);
	auto const network_graph = grenade::vx::network::build_network_graph(network, routing_result);

	// generate input
	grenade::vx::IODataMap inputs;
	constexpr size_t num = 100;
	constexpr size_t isi = 1000;
	std::vector<grenade::vx::TimedSpikeSequence> input_spikes(256);
	for (size_t i = 0; i < population_external.size; ++i) {
		for (size_t j = 0; j < num; ++j) {
			assert(network_graph.get_spike_labels().at(population_external_descriptor).at(i).at(0));
			grenade::vx::TimedSpike spike{Timer::Value(j * isi),
			                              SpikePack1ToChip(SpikePack1ToChip::labels_type{
			                                  *(network_graph.get_spike_labels()
			                                        .at(population_external_descriptor)
			                                        .at(i)
			                                        .at(0))})};
			input_spikes.at(i).push_back(spike);
		}
		inputs.runtime[grenade::vx::coordinate::ExecutionInstance()].push_back(
		    Timer::Value(num * isi));
	}
	assert(network_graph.get_event_input_vertex());
	inputs.data[*network_graph.get_event_input_vertex()] = std::move(input_spikes);

	// run graph with given inputs and return results
	auto const result_map =
	    grenade::vx::run(executor, network_graph.get_graph(), inputs, chip_configs);

	assert(network_graph.get_event_output_vertex());
	auto const result = std::get<std::vector<grenade::vx::TimedSpikeFromChipSequence>>(
	    result_map.data.at(*network_graph.get_event_output_vertex()));

	EXPECT_EQ(
	    result.size(), std::get<std::vector<grenade::vx::TimedSpikeSequence>>(
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
				if (spike.label == *expected_label) {
					matching++;
				} else {
					LOG4CXX_INFO(
					    logger, spike << spike.label.get_neuron_backend_address_out()
					                  << spike.label.get_neuron_event_output());
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
	auto [chip_config, connection] = initialize_excitatory_bypass();
	grenade::vx::JITGraphExecutor executor;
	executor.acquire_connection(DLSGlobal(), std::move(connection));

	grenade::vx::coordinate::ExecutionInstance instance;

	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[grenade::vx::coordinate::ExecutionInstance()] = chip_config;

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
		grenade::vx::network::NetworkBuilder network_builder;

		grenade::vx::network::ExternalPopulation population_external{1};
		auto const population_external_descriptor = network_builder.add(population_external);

		std::vector<grenade::vx::network::PopulationDescriptor> population_internal_descriptors;
		for (size_t n = 0; n < length; ++n) {
			grenade::vx::network::Population::Neurons neurons{all_neurons.at(n)};
			grenade::vx::network::Population population_internal{std::move(neurons),
			                                                     {n == length - 1}};
			population_internal_descriptors.push_back(network_builder.add(population_internal));
		}

		for (size_t i = 0; i < population_internal_descriptors.size(); ++i) {
			grenade::vx::network::Projection::Connections projection_connections{
			    {0, 0,
			     grenade::vx::network::Projection::Connection::Weight(
			         grenade::vx::network::Projection::Connection::Weight::max)}};
			if (i == 0) {
				grenade::vx::network::Projection projection{
				    grenade::vx::network::Projection::ReceptorType::excitatory,
				    std::move(projection_connections), population_external_descriptor,
				    population_internal_descriptors.at(0)};
				network_builder.add(projection);
			} else {
				grenade::vx::network::Projection projection{
				    grenade::vx::network::Projection::ReceptorType::excitatory,
				    std::move(projection_connections), population_internal_descriptors.at(i - 1),
				    population_internal_descriptors.at(i)};
				network_builder.add(projection);
			}
		}

		auto const network = network_builder.done();

		// build network graph
		auto const routing_result = grenade::vx::network::build_routing(network);
		auto const network_graph =
		    grenade::vx::network::build_network_graph(network, routing_result);

		// generate input
		grenade::vx::IODataMap inputs;
		constexpr size_t num = 100;
		constexpr size_t isi = 12500;
		std::vector<grenade::vx::TimedSpikeSequence> input_spikes(1);
		for (size_t j = 0; j < num; ++j) {
			assert(network_graph.get_spike_labels().at(population_external_descriptor).at(0).at(0));
			grenade::vx::TimedSpike spike{Timer::Value(j * isi),
			                              SpikePack1ToChip(SpikePack1ToChip::labels_type{
			                                  *(network_graph.get_spike_labels()
			                                        .at(population_external_descriptor)
			                                        .at(0)
			                                        .at(0))})};
			input_spikes.at(0).push_back(spike);
		}
		inputs.runtime[grenade::vx::coordinate::ExecutionInstance()].push_back(
		    Timer::Value(num * isi));
		assert(network_graph.get_event_input_vertex());
		inputs.data[*network_graph.get_event_input_vertex()] = std::move(input_spikes);

		// run graph with given inputs and return results
		auto const result_map =
		    grenade::vx::run(executor, network_graph.get_graph(), inputs, chip_configs);

		assert(network_graph.get_event_output_vertex());
		auto const result = std::get<std::vector<grenade::vx::TimedSpikeFromChipSequence>>(
		    result_map.data.at(*network_graph.get_event_output_vertex()));

		EXPECT_EQ(
		    result.size(), std::get<std::vector<grenade::vx::TimedSpikeSequence>>(
		                       inputs.data.at(*network_graph.get_event_input_vertex()))
		                       .size());
		auto const& spikes = result.at(0);
		// count correct spikes
		size_t matching = 0;
		auto const expected_label =
		    network_graph.get_spike_labels().at(population_internal_descriptors.back()).at(0).at(0);
		assert(expected_label);
		for (auto const& spike : spikes) {
			if (spike.label == *expected_label) {
				matching++;
			}
		}
		EXPECT_GE(matching, num * 0.8) << "length: " << length << " for seed: " << seed;
		EXPECT_LE(matching, num * 1.2) << "length: " << length << " for seed: " << seed;
	}
}
