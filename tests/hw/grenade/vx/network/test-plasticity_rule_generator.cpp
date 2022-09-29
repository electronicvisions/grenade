#include "grenade/vx/backend/connection.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/network/extract_output.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/plasticity_rule_generator.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/routing_builder.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include <gtest/gtest.h>
#include <log4cxx/logger.h>

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace stadls::vx::v3;
using namespace lola::vx::v3;
using namespace haldls::vx::v3;

TEST(OnlyRecordingPlasticityRuleGenerator, weights)
{
	grenade::vx::coordinate::ExecutionInstance instance;

	grenade::vx::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[instance] = lola::vx::v3::Chip();

	// build network
	grenade::vx::network::NetworkBuilder network_builder;

	// population at beginning of row
	grenade::vx::network::Population::Neurons neurons_1{AtomicNeuronOnDLS()};
	grenade::vx::network::Population::EnableRecordSpikes enable_record_spikes_1{false};
	grenade::vx::network::Population population_1{
	    std::move(neurons_1), std::move(enable_record_spikes_1)};
	auto const population_descriptor_1 = network_builder.add(population_1);

	// population not at beginning of row
	grenade::vx::network::Population::Neurons neurons_2{
	    AtomicNeuronOnDLS(NeuronColumnOnDLS(1), NeuronRowOnDLS(0))};
	grenade::vx::network::Population::EnableRecordSpikes enable_record_spikes_2{false};
	grenade::vx::network::Population population_2{
	    std::move(neurons_2), std::move(enable_record_spikes_2)};
	auto const population_descriptor_2 = network_builder.add(population_2);

	// population on bottom row
	grenade::vx::network::Population::Neurons neurons_4{
	    AtomicNeuronOnDLS(NeuronColumnOnDLS(1), NeuronRowOnDLS(1))};
	grenade::vx::network::Population::EnableRecordSpikes enable_record_spikes_4{false};
	grenade::vx::network::Population population_4{
	    std::move(neurons_4), std::move(enable_record_spikes_4)};
	auto const population_descriptor_4 = network_builder.add(population_4);

	// population on both rows
	grenade::vx::network::Population::Neurons neurons_5{
	    AtomicNeuronOnDLS(NeuronColumnOnDLS(2), NeuronRowOnDLS(0)),
	    AtomicNeuronOnDLS(NeuronColumnOnDLS(2), NeuronRowOnDLS(1))};
	grenade::vx::network::Population::EnableRecordSpikes enable_record_spikes_5{false, false};
	grenade::vx::network::Population population_5{
	    std::move(neurons_5), std::move(enable_record_spikes_5)};
	auto const population_descriptor_5 = network_builder.add(population_5);

	// projection at beginning of row
	grenade::vx::network::Projection::Connections projection_connections_1;
	for (size_t i = 0; i < population_1.neurons.size(); ++i) {
		projection_connections_1.push_back(
		    {i, i, grenade::vx::network::Projection::Connection::Weight(1)});
	}
	grenade::vx::network::Projection projection_1{
	    grenade::vx::network::Projection::ReceptorType::excitatory, projection_connections_1,
	    population_descriptor_1, population_descriptor_1};
	auto const projection_descriptor_1 = network_builder.add(projection_1);

	// projection not at beginning of row
	grenade::vx::network::Projection::Connections projection_connections_2;
	for (size_t i = 0; i < population_2.neurons.size(); ++i) {
		projection_connections_2.push_back(
		    {i, i, grenade::vx::network::Projection::Connection::Weight(2)});
	}
	grenade::vx::network::Projection projection_2{
	    grenade::vx::network::Projection::ReceptorType::excitatory, projection_connections_2,
	    population_descriptor_2, population_descriptor_2};
	auto const projection_descriptor_2 = network_builder.add(projection_2);

	// projection not at beginning of column
	grenade::vx::network::Projection::Connections projection_connections_3;
	for (size_t i = 0; i < population_1.neurons.size(); ++i) {
		projection_connections_3.push_back(
		    {i, i, grenade::vx::network::Projection::Connection::Weight(3)});
	}
	grenade::vx::network::Projection projection_3{
	    grenade::vx::network::Projection::ReceptorType::excitatory, projection_connections_3,
	    population_descriptor_1, population_descriptor_1};
	auto const projection_descriptor_3 = network_builder.add(projection_3);

	// projection on bottom hemisphere
	grenade::vx::network::Projection::Connections projection_connections_4;
	for (size_t i = 0; i < population_4.neurons.size(); ++i) {
		projection_connections_4.push_back(
		    {i, i, grenade::vx::network::Projection::Connection::Weight(4)});
	}
	grenade::vx::network::Projection projection_4{
	    grenade::vx::network::Projection::ReceptorType::excitatory, projection_connections_4,
	    population_descriptor_4, population_descriptor_4};
	auto const projection_descriptor_4 = network_builder.add(projection_4);

	// projection over two hemispheres
	grenade::vx::network::Projection::Connections projection_connections_5;
	for (size_t i = 0; i < population_5.neurons.size(); ++i) {
		projection_connections_5.push_back(
		    {0, i, grenade::vx::network::Projection::Connection::Weight(5 + i)});
	}
	grenade::vx::network::Projection projection_5{
	    grenade::vx::network::Projection::ReceptorType::excitatory, projection_connections_5,
	    population_descriptor_5, population_descriptor_5};
	auto const projection_descriptor_5 = network_builder.add(projection_5);


	grenade::vx::network::OnlyRecordingPlasticityRuleGenerator recording_generator(
	    {grenade::vx::network::OnlyRecordingPlasticityRuleGenerator::Observable::weights});

	grenade::vx::network::PlasticityRule plasticity_rule = recording_generator.generate();
	plasticity_rule.timer = grenade::vx::network::PlasticityRule::Timer{
	    grenade::vx::network::PlasticityRule::Timer::Value(0),
	    grenade::vx::network::PlasticityRule::Timer::Value(
	        Timer::Value::fpga_clock_cycles_per_us * 300),
	    2};
	plasticity_rule.projections = std::vector{
	    projection_descriptor_1, projection_descriptor_2, projection_descriptor_3,
	    projection_descriptor_4, projection_descriptor_5};

	auto const plasticity_rule_descriptor = network_builder.add(plasticity_rule);

	auto const network = network_builder.done();

	auto const routing_result = grenade::vx::network::build_routing(network);
	auto const network_graph = grenade::vx::network::build_network_graph(network, routing_result);

	grenade::vx::IODataMap inputs;
	inputs.runtime[instance].push_back(Timer::Value(Timer::Value::fpga_clock_cycles_per_us * 1000));

	// Construct connection to HW
	grenade::vx::backend::Connection connection;
	std::map<DLSGlobal, grenade::vx::backend::Connection> connections;
	connections.emplace(DLSGlobal(), std::move(connection));
	grenade::vx::JITGraphExecutor executor(std::move(connections));

	// run graph with given inputs and return results
	auto const result_map =
	    grenade::vx::run(executor, network_graph.get_graph(), inputs, chip_configs);

	auto const recorded_data = std::get<grenade::vx::network::PlasticityRule::TimedRecordingData>(
	    grenade::vx::network::extract_plasticity_rule_recording_data(
	        result_map, network_graph, plasticity_rule_descriptor));
	EXPECT_EQ(recorded_data.data_per_synapse.size(), 1);
	EXPECT_EQ(recorded_data.data_array.size(), 0);
	EXPECT_TRUE(recorded_data.data_per_synapse.contains("weights"));
	EXPECT_EQ(recorded_data.data_per_synapse.at("weights").size(), 5 /* #projections */);
	for (auto const& [descriptor, ws] : recorded_data.data_per_synapse.at("weights")) {
		auto const& weights =
		    std::get<std::vector<grenade::vx::TimedDataSequence<std::vector<int8_t>>>>(ws);
		EXPECT_EQ(weights.size(), inputs.batch_size());
		for (size_t i = 0; i < weights.size(); ++i) {
			auto const& samples = weights.at(i);
			for (auto const& sample : samples) {
				if (descriptor == projection_descriptor_5) {
					EXPECT_EQ(sample.data.size(), 2 /* #connections/projection */);
				} else {
					EXPECT_EQ(sample.data.size(), 1 /* #connections/projection */);
				}
				for (size_t i = 0; i < sample.data.size(); ++i) {
					EXPECT_EQ(
					    static_cast<int>(sample.data.at(i)), network_graph.get_network()
					                                             ->projections.at(descriptor)
					                                             .connections.at(i)
					                                             .weight);
				}
			}
		}
	}
}