#include <gtest/gtest.h>

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/backend/run.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/network/extract_output.h"
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
#include "haldls/vx/v3/systime.h"
#include "haldls/vx/v3/timer.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/init_generator.h"
#include "stadls/vx/v3/playback_generator.h"
#include "stadls/vx/v3/run.h"
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

void test_background_spike_source_regular(
    BackgroundSpikeSource::Period period,
    Timer::Value running_period,
    size_t spike_count_deviation,
    grenade::vx::JITGraphExecutor::Connections const& connections,
    grenade::vx::JITGraphExecutor::ChipConfigs const& chip_configs)
{
	size_t expected_count =
	    running_period * 2 /* f(FPGA) = 0.5 * f(BackgroundSpikeSource) */ / period;

	typed_array<SpikeLabel, BackgroundSpikeSourceOnDLS> expected_labels;

	grenade::vx::coordinate::ExecutionInstance instance;

	// build network
	grenade::vx::network::NetworkBuilder network_builder;

	grenade::vx::network::BackgroundSpikeSourcePopulation population_background_spike_source{
	    1,
	    {{HemisphereOnDLS(0), PADIBusOnPADIBusBlock(0)}},
	    grenade::vx::network::BackgroundSpikeSourcePopulation::Config{
	        BackgroundSpikeSource::Period(period), BackgroundSpikeSource::Rate(),
	        BackgroundSpikeSource::Seed(), false}};
	auto const population_background_spike_source_descriptor =
	    network_builder.add(population_background_spike_source);

	grenade::vx::network::Population::Neurons neurons{AtomicNeuronOnDLS()};
	grenade::vx::network::Population::EnableRecordSpikes enable_record_spikes{true};
	grenade::vx::network::Population population_internal{std::move(neurons), enable_record_spikes};
	auto const population_internal_descriptor = network_builder.add(population_internal);

	grenade::vx::network::Projection::Connections projection_connections;
	for (size_t i = 0; i < population_background_spike_source.size; ++i) {
		projection_connections.push_back(
		    {i, i, grenade::vx::network::Projection::Connection::Weight(63)});
	}
	grenade::vx::network::Projection projection{
	    grenade::vx::network::Projection::ReceptorType::excitatory,
	    std::move(projection_connections), population_background_spike_source_descriptor,
	    population_internal_descriptor};
	network_builder.add(projection);

	auto const network = network_builder.done();

	// build network graph
	auto const routing_result = grenade::vx::network::build_routing(network);
	auto const network_graph = grenade::vx::network::build_network_graph(network, routing_result);

	// generate input
	grenade::vx::IODataMap inputs;
	inputs.runtime.push_back(running_period);

	// run graph with given inputs and return results
	auto const result_map = grenade::vx::JITGraphExecutor::run(
	    network_graph.get_graph(), inputs, connections, chip_configs);

	auto const spikes = grenade::vx::network::extract_neuron_spikes(result_map, network_graph);
	EXPECT_EQ(spikes.size(), 1);

	size_t expected_label_count = spikes.at(0).contains(AtomicNeuronOnDLS())
	                                  ? spikes.at(0).at(AtomicNeuronOnDLS()).size()
	                                  : 0;

	// check approx. equality in number of spikes expected
	EXPECT_TRUE(
	    (expected_label_count <= (expected_count + spike_count_deviation)) &&
	    (expected_label_count >= (expected_count - spike_count_deviation)))
	    << "expected: " << expected_count << " actual: " << expected_label_count;
}

/**
 * Enable background sources and generate regular spike-trains.
 */
TEST(NetworkGraphBuilder, BackgroundSpikeSourceRegular)
{
	// Construct connection to HW
	auto [chip_config, connection] = initialize_excitatory_bypass();
	grenade::vx::JITGraphExecutor::Connections connections;
	connections.insert(
	    std::pair<DLSGlobal, grenade::vx::backend::Connection&>(DLSGlobal(), connection));
	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[DLSGlobal()] = chip_config;

	// 5% allowed deviation in spike count
	test_background_spike_source_regular(
	    BackgroundSpikeSource::Period(1000), Timer::Value(10000000), 1000, connections,
	    chip_configs);
	test_background_spike_source_regular(
	    BackgroundSpikeSource::Period(10000), Timer::Value(100000000), 1000, connections,
	    chip_configs);
}

void test_background_spike_source_poisson(
    BackgroundSpikeSource::Period period,
    BackgroundSpikeSource::Rate rate,
    Timer::Value running_period,
    intmax_t spike_count_deviation,
    grenade::vx::JITGraphExecutor::Connections const& connections,
    grenade::vx::JITGraphExecutor::ChipConfigs const& chip_configs)
{
	intmax_t expected_count = running_period * 2 /* f(FPGA) = 0.5 * f(BackgroundSpikeSource) */ /
	                          period / 64 /* population size */;

	typed_array<SpikeLabel, BackgroundSpikeSourceOnDLS> expected_labels;

	grenade::vx::coordinate::ExecutionInstance instance;

	grenade::vx::network::BackgroundSpikeSourcePopulation population_background_spike_source{
	    64,
	    {{HemisphereOnDLS(0), PADIBusOnPADIBusBlock(0)}},
	    grenade::vx::network::BackgroundSpikeSourcePopulation::Config{
	        BackgroundSpikeSource::Period(period), BackgroundSpikeSource::Rate(rate),
	        BackgroundSpikeSource::Seed(1234), true}};

	grenade::vx::network::Population::Neurons neurons;
	grenade::vx::network::Population::EnableRecordSpikes enable_record_spikes;
	for (size_t i = 0; i < population_background_spike_source.size; ++i) {
		neurons.push_back(AtomicNeuronOnDLS(NeuronColumnOnDLS(i), NeuronRowOnDLS()));
		enable_record_spikes.push_back(true);
	}
	grenade::vx::network::Population population_internal{std::move(neurons), enable_record_spikes};

	for (size_t i = 0; i < population_background_spike_source.size; ++i) {
		// build network
		grenade::vx::network::NetworkBuilder network_builder;

		auto const population_background_spike_source_descriptor =
		    network_builder.add(population_background_spike_source);

		auto const population_internal_descriptor = network_builder.add(population_internal);

		grenade::vx::network::Projection::Connections projection_connections;
		projection_connections.push_back(
		    {i, i, grenade::vx::network::Projection::Connection::Weight(63)});

		grenade::vx::network::Projection projection{
		    grenade::vx::network::Projection::ReceptorType::excitatory,
		    std::move(projection_connections), population_background_spike_source_descriptor,
		    population_internal_descriptor};
		network_builder.add(projection);

		auto const network = network_builder.done();

		// build network graph
		auto const routing_result = grenade::vx::network::build_routing(network);
		auto const network_graph =
		    grenade::vx::network::build_network_graph(network, routing_result);

		// generate input
		grenade::vx::IODataMap inputs;
		inputs.runtime.push_back(running_period);

		// run graph with given inputs and return results
		auto const result_map = grenade::vx::JITGraphExecutor::run(
		    network_graph.get_graph(), inputs, connections, chip_configs);

		auto const spikes = grenade::vx::network::extract_neuron_spikes(result_map, network_graph);
		EXPECT_EQ(spikes.size(), 1);

		std::array<intmax_t, 64> expected_label_counts;
		expected_label_counts.fill(0);
		for (auto [an, spike] : spikes.at(0)) {
			if (an.toNeuronRowOnDLS() == NeuronRowOnDLS()) {
				expected_label_counts.at(an.toNeuronColumnOnDLS().value()) += spike.size();
			}
		}

		// check approx. equality in number of spikes expected
		size_t j = 0;
		for (auto const expected_label_count : expected_label_counts) {
			if (i == j) {
				EXPECT_TRUE(
				    (expected_label_count <= (expected_count + spike_count_deviation)) &&
				    (expected_label_count >= (expected_count - spike_count_deviation)))
				    << "expected: " << expected_count << " actual: " << expected_label_count
				    << "at: " << NeuronColumnOnDLS(i);
			} else {
				EXPECT_TRUE(
				    (expected_label_count <= (0 + spike_count_deviation)) &&
				    (expected_label_count >= (0 - spike_count_deviation)))
				    << "expected: " << 0 << " actual: " << expected_label_count
				    << "at: " << NeuronColumnOnDLS(i);
			}
			j++;
		}
	}
}

/**
 * Enable background sources and generate regular spike-trains.
 */
TEST(NetworkGraphBuilder, BackgroundSpikeSourcePoisson)
{
	// Construct connection to HW
	auto [chip_config, connection] = initialize_excitatory_bypass();
	grenade::vx::JITGraphExecutor::Connections connections;
	connections.insert(
	    std::pair<DLSGlobal, grenade::vx::backend::Connection&>(DLSGlobal(), connection));
	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[DLSGlobal()] = chip_config;

	// 5% allowed deviation in spike count
	test_background_spike_source_poisson(
	    BackgroundSpikeSource::Period(1000), BackgroundSpikeSource::Rate(255),
	    Timer::Value(10000000), 1000, connections, chip_configs);
	test_background_spike_source_poisson(
	    BackgroundSpikeSource::Period(10000), BackgroundSpikeSource::Rate(255),
	    Timer::Value(100000000), 1000, connections, chip_configs);
}
