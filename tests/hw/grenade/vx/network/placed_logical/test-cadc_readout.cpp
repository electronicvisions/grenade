#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/backend/run.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/placed_atomic/build_routing.h"
#include "grenade/vx/network/placed_atomic/network_graph_builder.h"
#include "grenade/vx/network/placed_logical/cadc_recording.h"
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

TEST(CADCRecording, General)
{
	// Construct connection to HW
	auto chip_configs = get_chip_configs_bypass_excitatory();
	grenade::vx::execution::JITGraphExecutor executor;

	grenade::vx::signal_flow::ExecutionInstance instance;

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
	cadc_recording.neurons.push_back(CADCRecording::Neuron{
	    population_internal_descriptor, 14, CompartmentOnLogicalNeuron(), 0,
	    CADCRecording::Neuron::Source::membrane});
	cadc_recording.neurons.push_back(CADCRecording::Neuron{
	    population_internal_descriptor, 60, CompartmentOnLogicalNeuron(), 0,
	    CADCRecording::Neuron::Source::membrane});
	cadc_recording.neurons.push_back(CADCRecording::Neuron{
	    population_internal_descriptor, 25, CompartmentOnLogicalNeuron(), 0,
	    CADCRecording::Neuron::Source::membrane});
	cadc_recording.neurons.push_back(CADCRecording::Neuron{
	    population_internal_descriptor, 150, CompartmentOnLogicalNeuron(), 0,
	    CADCRecording::Neuron::Source::membrane});
	network_builder.add(cadc_recording);

	auto const network = network_builder.done();
	auto const network_graph = build_network_graph(network);
	auto const routing_result = build_routing(network_graph.get_hardware_network());
	auto const atomic_network_graph = grenade::vx::network::placed_atomic::build_network_graph(
	    network_graph.get_hardware_network(), routing_result);

	grenade::vx::signal_flow::IODataMap inputs;
	inputs.runtime.push_back(
	    {{instance,
	      grenade::vx::common::Time(grenade::vx::common::Time::fpga_clock_cycles_per_us * 100)}});
	inputs.runtime.push_back(
	    {{instance,
	      grenade::vx::common::Time(grenade::vx::common::Time::fpga_clock_cycles_per_us * 150)}});

	// run graph with given inputs and return results
	auto const result_map = grenade::vx::execution::run(
	    executor, atomic_network_graph.get_graph(), inputs, chip_configs);

	assert(atomic_network_graph.get_cadc_sample_output_vertex().size());
	EXPECT_EQ(atomic_network_graph.get_cadc_sample_output_vertex().size(), 1);
	auto const result = std::get<std::vector<
	    grenade::vx::common::TimedDataSequence<std::vector<grenade::vx::signal_flow::Int8>>>>(
	    result_map.data.at(atomic_network_graph.get_cadc_sample_output_vertex().at(0)));

	EXPECT_EQ(result.size(), inputs.batch_size());
	std::set<grenade::vx::signal_flow::Int8> unique_values;
	for (size_t i = 0; i < result.size(); ++i) {
		auto const& samples = result.at(i);
		for (auto const& sample : samples) {
			EXPECT_EQ(sample.data.size(), cadc_recording.neurons.size());
			for (auto const& e : sample.data) {
				unique_values.insert(e);
			}
		}
		// CADC sampling shall take between one and two us
		EXPECT_GE(
		    samples.size(),
		    inputs.runtime.at(i).at(instance) / Timer::Value::fpga_clock_cycles_per_us / 2);
		EXPECT_LE(
		    samples.size(),
		    inputs.runtime.at(i).at(instance) / Timer::Value::fpga_clock_cycles_per_us);
	}
	EXPECT_GT(unique_values.size(), 1);
}
