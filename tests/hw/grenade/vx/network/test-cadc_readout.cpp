#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/network/cadc_recording.h"
#include "grenade/vx/network/extract_output.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/routing/portfolio_router.h"
#include "grenade/vx/network/run.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/timer.h"
#include "helper.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
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

TEST(CADCRecording, General)
{
	// Construct connection to HW
	auto chip_configs = get_chip_configs_bypass_excitatory();

	grenade::common::ExecutionInstanceID instance;

	// CADC sampling shall take between one and {2.5, 7.0} us depending on placement of recording
	// data
	std::map<bool, float> expected_period = {{false, 2.5}, {true, 7.0}};

	for (auto const placement_on_dram : {false, true}) {
		grenade::vx::execution::JITGraphExecutor executor;

		// build network
		NetworkBuilder network_builder;

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
			             Population::Neuron::Compartment::SpikeMaster(0, false),
			             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}));
		}
		std::mt19937 rng{std::random_device{}()};
		std::shuffle(neurons.begin(), neurons.end(), rng);
		Population population_internal{std::move(neurons)};
		auto const population_internal_descriptor = network_builder.add(population_internal);

		CADCRecording cadc_recording;
		cadc_recording.placement_on_dram = placement_on_dram;
		cadc_recording.neurons.push_back(CADCRecording::Neuron{
		    AtomicNeuronOnExecutionInstance(
		        population_internal_descriptor, 14, CompartmentOnLogicalNeuron(), 0),
		    CADCRecording::Neuron::Source::membrane});
		cadc_recording.neurons.push_back(CADCRecording::Neuron{
		    AtomicNeuronOnExecutionInstance(
		        population_internal_descriptor, 60, CompartmentOnLogicalNeuron(), 0),
		    CADCRecording::Neuron::Source::membrane});
		cadc_recording.neurons.push_back(CADCRecording::Neuron{
		    AtomicNeuronOnExecutionInstance(
		        population_internal_descriptor, 25, CompartmentOnLogicalNeuron(), 0),
		    CADCRecording::Neuron::Source::membrane});
		cadc_recording.neurons.push_back(CADCRecording::Neuron{
		    AtomicNeuronOnExecutionInstance(
		        population_internal_descriptor, 150, CompartmentOnLogicalNeuron(), 0),
		    CADCRecording::Neuron::Source::membrane});
		network_builder.add(cadc_recording);

		auto const network = network_builder.done();
		auto const routing_result = routing::PortfolioRouter()(network);
		auto const network_graph = build_network_graph(network, routing_result);

		grenade::vx::signal_flow::InputData inputs;
		inputs.runtime.push_back(
		    {{instance, grenade::vx::common::Time(
		                    grenade::vx::common::Time::fpga_clock_cycles_per_us * 100 *
		                    (placement_on_dram ? 10 : 1))}});
		inputs.runtime.push_back(
		    {{instance, grenade::vx::common::Time(
		                    grenade::vx::common::Time::fpga_clock_cycles_per_us * 150 *
		                    (placement_on_dram ? 10 : 1))}});

		// run graph with given inputs and return results
		auto const result_map = run(executor, network_graph, chip_configs, inputs);

		auto const result = extract_cadc_samples(result_map, network_graph);

		EXPECT_EQ(result.size(), inputs.batch_size());
		std::set<grenade::vx::common::Time> unique_times;
		for (size_t i = 0; i < result.size(); ++i) {
			std::map<AtomicNeuronOnNetwork, size_t> samples_per_neuron;
			auto const& samples = result.at(i);
			for (auto const& sample : samples) {
				auto const& [t, an_on_network, _] = sample;
				samples_per_neuron[an_on_network] += 1;
				unique_times.insert(t);
			}
			EXPECT_EQ(samples_per_neuron.size(), cadc_recording.neurons.size())
			    << placement_on_dram;
			for (auto const& [_, num] : samples_per_neuron) {
				EXPECT_GE(
				    num, inputs.runtime.at(i).at(instance) /
				             Timer::Value::fpga_clock_cycles_per_us /
				             expected_period.at(placement_on_dram))
				    << placement_on_dram;
				EXPECT_LE(
				    num, inputs.runtime.at(i).at(instance) / Timer::Value::fpga_clock_cycles_per_us)
				    << placement_on_dram;
			}
		}
		// only test that time is recorded and not all zeros
		EXPECT_GT(unique_times.size(), 1);
	}
}
