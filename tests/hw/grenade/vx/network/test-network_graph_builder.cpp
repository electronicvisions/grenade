#include "grenade/vx/backend/connection.h"
#include "grenade/vx/backend/run.h"
#include "grenade/vx/config.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/network/connection_builder.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "haldls/vx/v2/neuron.h"
#include "haldls/vx/v2/timer.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
#include "stadls/vx/v2/init_generator.h"
#include "stadls/vx/v2/playback_generator.h"
#include "stadls/vx/v2/run.h"
#include <gtest/gtest.h>
#include <log4cxx/logger.h>

using namespace halco::common;
using namespace halco::hicann_dls::vx::v2;
using namespace stadls::vx::v2;
using namespace lola::vx::v2;
using namespace haldls::vx::v2;

std::pair<grenade::vx::ChipConfig, grenade::vx::backend::Connection> initialize_excitatory_bypass()
{
	std::unique_ptr<grenade::vx::ChipConfig> chip = std::make_unique<grenade::vx::ChipConfig>();
	// Initialize chip
		ExperimentInit init;
		for (auto const c : iter_all<CommonNeuronBackendConfigOnDLS>()) {
			chip->neuron_backend[c].set_enable_clocks(true);
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
	    stadls::vx::v2::PlaybackProgramBuilder builder;
	    // enable excitatory bypass mode
	    for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
		    auto& config = chip->hemispheres[neuron.toNeuronRowOnDLS().toHemisphereOnDLS()]
		                       .neuron_block[neuron.toNeuronColumnOnDLS()];
		    config.event_routing.enable_digital = true;
		    config.event_routing.analog_output =
		        lola::vx::v2::AtomicNeuron::EventRouting::AnalogOutputMode::normal;
		    config.event_routing.enable_bypass_excitatory = true;
		    config.threshold.enable = false;
	    }
	    for (auto const block : iter_all<CommonPADIBusConfigOnDLS>()) {
		    auto& padi_config = chip->hemispheres[block.toHemisphereOnDLS()].common_padi_bus_config;
		    auto dacen = padi_config.get_dacen_pulse_extension();
		    for (auto const block : iter_all<PADIBusOnPADIBusBlock>()) {
			    dacen[block] = CommonPADIBusConfig::DacenPulseExtension(15);
		    }
		    padi_config.set_dacen_pulse_extension(dacen);
	    }
	    for (auto const drv : iter_all<SynapseDriverOnDLS>()) {
		    auto& config = chip->hemispheres[drv.toSynapseDriverBlockOnDLS().toHemisphereOnDLS()]
		                       .synapse_driver_block[drv.toSynapseDriverOnSynapseDriverBlock()];
		    config.set_enable_receiver(true);
		    config.set_enable_address_out(true);
	    }
	    for (auto const block : iter_all<CapMemBlockOnDLS>()) {
		    CapMemCell cell(CapMemCell::Value(1022));
		    builder.write(CapMemCellOnDLS(CapMemCellOnCapMemBlock::syn_i_bias_dac, block), cell);
	    }
	    auto program = builder.done();
	    grenade::vx::backend::run(connection, program);
	    return std::make_pair(*chip, std::move(connection));
}

TEST(NetworkGraphBuilder, FeedForwardOneToOne)
{
	// Construct connection to HW
	auto [chip_config, connection] = initialize_excitatory_bypass();
	grenade::vx::JITGraphExecutor::Connections connections;
	connections.insert(
	    std::pair<DLSGlobal, grenade::vx::backend::Connection&>(DLSGlobal(), connection));

	grenade::vx::coordinate::ExecutionInstance instance;

	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[DLSGlobal()] = chip_config;

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
			grenade::vx::TimedSpike spike{Timer::Value(j * isi),
			                              SpikePack1ToChip(SpikePack1ToChip::labels_type{
			                                  network_graph.get_spike_labels()
			                                      .at(population_external_descriptor)
			                                      .at(i)
			                                      .at(0)})};
			input_spikes.at(i).push_back(spike);
		}
		inputs.runtime.push_back(Timer::Value(num * isi));
	}
	assert(network_graph.get_event_input_vertex());
	inputs.data[*network_graph.get_event_input_vertex()] = std::move(input_spikes);

	// run graph with given inputs and return results
	auto const result_map = grenade::vx::JITGraphExecutor::run(
	    network_graph.get_graph(), inputs, connections, chip_configs);

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
		for (auto const spike : spikes) {
			if (spike.get_label() != expected_label) {
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
	grenade::vx::JITGraphExecutor::Connections connections;
	connections.insert(
	    std::pair<DLSGlobal, grenade::vx::backend::Connection&>(DLSGlobal(), connection));

	grenade::vx::coordinate::ExecutionInstance instance;

	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[DLSGlobal()] = chip_config;

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
			grenade::vx::TimedSpike spike{Timer::Value(j * isi),
			                              SpikePack1ToChip(SpikePack1ToChip::labels_type{
			                                  network_graph.get_spike_labels()
			                                      .at(population_external_descriptor)
			                                      .at(i)
			                                      .at(0)})};
			input_spikes.at(i).push_back(spike);
		}
		inputs.runtime.push_back(Timer::Value(num * isi));
	}
	assert(network_graph.get_event_input_vertex());
	inputs.data[*network_graph.get_event_input_vertex()] = std::move(input_spikes);

	// run graph with given inputs and return results
	auto const result_map = grenade::vx::JITGraphExecutor::run(
	    network_graph.get_graph(), inputs, connections, chip_configs);

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
			for (auto const& spike : spikes) {
				if (spike.get_label() == expected_label) {
					matching++;
				} else {
					LOG4CXX_INFO(
					    logger, spike << spike.get_label().get_neuron_backend_address_out()
					                  << spike.get_label().get_neuron_event_output());
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
	grenade::vx::JITGraphExecutor::Connections connections;
	connections.insert(
	    std::pair<DLSGlobal, grenade::vx::backend::Connection&>(DLSGlobal(), connection));

	grenade::vx::coordinate::ExecutionInstance instance;

	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[DLSGlobal()] = chip_config;

	// build network
	grenade::vx::network::NetworkBuilder network_builder;

	grenade::vx::network::ExternalPopulation population_external{1};
	auto const population_external_descriptor = network_builder.add(population_external);

	std::vector<grenade::vx::network::PopulationDescriptor> population_internal_descriptors;
	for (auto const column : iter_all<NeuronColumnOnDLS>()) {
		// skip parts of the columns to allow placement of the synapse driver for external input
		if (column >= NeuronColumnOnDLS::max - (128 + 48)) {
			continue;
		}
		grenade::vx::network::Population::Neurons neurons{
		    AtomicNeuronOnDLS(column, NeuronRowOnDLS::top)};
		grenade::vx::network::Population population_internal{std::move(neurons), {true}};
		population_internal_descriptors.push_back(network_builder.add(population_internal));
	}

	for (size_t i = 0; i < population_internal_descriptors.size(); ++i) {
		grenade::vx::network::Projection::Connections projection_connections{
		    {0, 0, grenade::vx::network::Projection::Connection::Weight(63)}};
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
	auto const network_graph = grenade::vx::network::build_network_graph(network, routing_result);

	// generate input
	grenade::vx::IODataMap inputs;
	constexpr size_t num = 100;
	constexpr size_t isi = 1000;
	std::vector<grenade::vx::TimedSpikeSequence> input_spikes(1);
	for (size_t j = 0; j < num; ++j) {
		grenade::vx::TimedSpike spike{
		    Timer::Value(j * isi),
		    SpikePack1ToChip(SpikePack1ToChip::labels_type{
		        network_graph.get_spike_labels().at(population_external_descriptor).at(0).at(0)})};
		input_spikes.at(0).push_back(spike);
	}
	inputs.runtime.push_back(Timer::Value(num * isi));
	assert(network_graph.get_event_input_vertex());
	inputs.data[*network_graph.get_event_input_vertex()] = std::move(input_spikes);

	// run graph with given inputs and return results
	auto const result_map = grenade::vx::JITGraphExecutor::run(
	    network_graph.get_graph(), inputs, connections, chip_configs);

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
	for (auto const& spike : spikes) {
		if (spike.get_label() == expected_label) {
			matching++;
		}
	}
	EXPECT_GE(matching, num * 0.8);
	EXPECT_LE(matching, num * 1.2);
}
