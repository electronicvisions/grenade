#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/network/extract_output.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/plasticity_rule_generator.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/routing/portfolio_router.h"
#include "grenade/vx/network/run.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include <gtest/gtest.h>
#include <log4cxx/logger.h>

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace stadls::vx::v3;
using namespace lola::vx::v3;
using namespace haldls::vx::v3;
using namespace grenade::vx::network;

TEST(OnlyRecordingPlasticityRuleGenerator, weights)
{
	grenade::common::ExecutionInstanceID instance;

	grenade::vx::execution::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[instance] = lola::vx::v3::Chip();

	// build network
	NetworkBuilder network_builder;

	// population at beginning of row
	Population::Neurons neurons_1{Population::Neuron(
	    LogicalNeuronOnDLS(
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	        AtomicNeuronOnDLS()),
	    Population::Neuron::Compartments{
	        {CompartmentOnLogicalNeuron(),
	         Population::Neuron::Compartment{
	             Population::Neuron::Compartment::SpikeMaster(0, false),
	             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})};
	Population population_1{std::move(neurons_1)};
	auto const population_descriptor_1 = network_builder.add(population_1);

	// population not at beginning of row
	Population::Neurons neurons_2{Population::Neuron(
	    LogicalNeuronOnDLS(
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	        AtomicNeuronOnDLS(NeuronColumnOnDLS(1), NeuronRowOnDLS(0))),
	    Population::Neuron::Compartments{
	        {CompartmentOnLogicalNeuron(),
	         Population::Neuron::Compartment{
	             Population::Neuron::Compartment::SpikeMaster(0, false),
	             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})};
	Population population_2{std::move(neurons_2)};
	auto const population_descriptor_2 = network_builder.add(population_2);

	// population on bottom row
	Population::Neurons neurons_4{Population::Neuron(
	    LogicalNeuronOnDLS(
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	        AtomicNeuronOnDLS(NeuronColumnOnDLS(1), NeuronRowOnDLS(1))),
	    Population::Neuron::Compartments{
	        {CompartmentOnLogicalNeuron(),
	         Population::Neuron::Compartment{
	             Population::Neuron::Compartment::SpikeMaster(0, false),
	             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})};
	Population population_4{std::move(neurons_4)};
	auto const population_descriptor_4 = network_builder.add(population_4);

	// population on both rows
	Population::Neurons neurons_5{
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(NeuronColumnOnDLS(2), NeuronRowOnDLS(0))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}),
	    Population::Neuron(
	        LogicalNeuronOnDLS(
	            LogicalNeuronCompartments(
	                {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	            AtomicNeuronOnDLS(NeuronColumnOnDLS(2), NeuronRowOnDLS(1))),
	        Population::Neuron::Compartments{
	            {CompartmentOnLogicalNeuron(),
	             Population::Neuron::Compartment{
	                 Population::Neuron::Compartment::SpikeMaster(0, false),
	                 {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})};
	Population population_5{std::move(neurons_5)};
	auto const population_descriptor_5 = network_builder.add(population_5);

	// projection at beginning of row
	Projection::Connections projection_connections_1;
	for (size_t i = 0; i < population_1.neurons.size(); ++i) {
		projection_connections_1.push_back(
		    {{i, CompartmentOnLogicalNeuron()},
		     {i, CompartmentOnLogicalNeuron()},
		     Projection::Connection::Weight(1)});
	}
	Projection projection_1{
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), projection_connections_1,
	    population_descriptor_1.toPopulationOnExecutionInstance(),
	    population_descriptor_1.toPopulationOnExecutionInstance()};
	auto const projection_descriptor_1 = network_builder.add(projection_1);

	// projection not at beginning of row
	Projection::Connections projection_connections_2;
	for (size_t i = 0; i < population_2.neurons.size(); ++i) {
		projection_connections_2.push_back(
		    {{i, CompartmentOnLogicalNeuron()},
		     {i, CompartmentOnLogicalNeuron()},
		     Projection::Connection::Weight(2)});
	}
	Projection projection_2{
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), projection_connections_2,
	    population_descriptor_2.toPopulationOnExecutionInstance(),
	    population_descriptor_2.toPopulationOnExecutionInstance()};
	auto const projection_descriptor_2 = network_builder.add(projection_2);

	// projection not at beginning of column
	Projection::Connections projection_connections_3;
	for (size_t i = 0; i < population_1.neurons.size(); ++i) {
		projection_connections_3.push_back(
		    {{i, CompartmentOnLogicalNeuron()},
		     {i, CompartmentOnLogicalNeuron()},
		     Projection::Connection::Weight(3)});
	}
	Projection projection_3{
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), projection_connections_3,
	    population_descriptor_1.toPopulationOnExecutionInstance(),
	    population_descriptor_1.toPopulationOnExecutionInstance()};
	auto const projection_descriptor_3 = network_builder.add(projection_3);

	// projection on bottom hemisphere
	Projection::Connections projection_connections_4;
	for (size_t i = 0; i < population_4.neurons.size(); ++i) {
		projection_connections_4.push_back(
		    {{i, CompartmentOnLogicalNeuron()},
		     {i, CompartmentOnLogicalNeuron()},
		     Projection::Connection::Weight(4)});
	}
	Projection projection_4{
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), projection_connections_4,
	    population_descriptor_4.toPopulationOnExecutionInstance(),
	    population_descriptor_4.toPopulationOnExecutionInstance()};
	auto const projection_descriptor_4 = network_builder.add(projection_4);

	// projection over two hemispheres
	Projection::Connections projection_connections_5;
	for (size_t i = 0; i < population_5.neurons.size(); ++i) {
		projection_connections_5.push_back(
		    {{0, CompartmentOnLogicalNeuron()},
		     {i, CompartmentOnLogicalNeuron()},
		     Projection::Connection::Weight(5 + i)});
	}
	Projection projection_5{
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), projection_connections_5,
	    population_descriptor_5.toPopulationOnExecutionInstance(),
	    population_descriptor_5.toPopulationOnExecutionInstance()};
	auto const projection_descriptor_5 = network_builder.add(projection_5);


	OnlyRecordingPlasticityRuleGenerator recording_generator(
	    {OnlyRecordingPlasticityRuleGenerator::Observable::weights});

	PlasticityRule plasticity_rule = recording_generator.generate();
	plasticity_rule.timer = PlasticityRule::Timer{
	    PlasticityRule::Timer::Value(0),
	    PlasticityRule::Timer::Value(Timer::Value::fpga_clock_cycles_per_us * 300), 2};
	plasticity_rule.projections = std::vector{
	    projection_descriptor_1.toProjectionOnExecutionInstance(),
	    projection_descriptor_2.toProjectionOnExecutionInstance(),
	    projection_descriptor_3.toProjectionOnExecutionInstance(),
	    projection_descriptor_4.toProjectionOnExecutionInstance(),
	    projection_descriptor_5.toProjectionOnExecutionInstance()};

	auto const plasticity_rule_descriptor = network_builder.add(plasticity_rule);

	auto const network = network_builder.done();
	auto const routing_result = routing::PortfolioRouter()(network);
	auto const network_graph = build_network_graph(network, routing_result);

	grenade::vx::signal_flow::InputData inputs;
	inputs.snippets.resize(1);
	inputs.snippets.at(0).runtime.push_back(
	    {{instance,
	      grenade::vx::common::Time(grenade::vx::common::Time::fpga_clock_cycles_per_us * 1000)}});

	// Construct connection to HW
	grenade::vx::execution::JITGraphExecutor executor;

	// run graph with given inputs and return results
	auto const result_map = run(executor, network_graph, chip_configs, inputs);

	auto const recorded_data =
	    std::get<PlasticityRule::TimedRecordingData>(extract_plasticity_rule_recording_data(
	        result_map.snippets.at(0), network_graph, plasticity_rule_descriptor));
	EXPECT_EQ(recorded_data.data_per_synapse.size(), 1);
	EXPECT_EQ(recorded_data.data_array.size(), 0);
	EXPECT_TRUE(recorded_data.data_per_synapse.contains("weights"));
	EXPECT_EQ(recorded_data.data_per_synapse.at("weights").size(), 5 /* #projections */);
	for (auto const& [descriptor, ws] : recorded_data.data_per_synapse.at("weights")) {
		auto const& weights = std::get<
		    std::vector<grenade::vx::common::TimedDataSequence<std::vector<std::vector<int8_t>>>>>(
		    ws);
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
					    static_cast<int>(sample.data.at(i).at(0)),
					    network_graph.get_network()
					        ->execution_instances.at(instance)
					        .projections.at(descriptor)
					        .connections.at(i)
					        .weight);
				}
			}
		}
	}
}
