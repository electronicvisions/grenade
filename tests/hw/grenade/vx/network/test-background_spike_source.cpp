#include <gtest/gtest.h>

#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/backend/run.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/build_routing.h"
#include "grenade/vx/network/extract_output.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/signal_flow/execution_instance.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/systime.h"
#include "haldls/vx/v3/timer.h"
#include "helper.h"
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
using namespace grenade::vx::network;

void test_background_spike_source_regular(
    BackgroundSpikeSource::Period period,
    grenade::vx::common::Time running_period,
    size_t spike_count_deviation,
    grenade::vx::execution::JITGraphExecutor& executor,
    grenade::vx::execution::JITGraphExecutor::ChipConfigs const& chip_configs)
{
	size_t expected_count =
	    running_period * 2 /* f(FPGA) = 0.5 * f(BackgroundSpikeSource) */ / period;

	typed_array<SpikeLabel, BackgroundSpikeSourceOnDLS> expected_labels;

	grenade::vx::signal_flow::ExecutionInstance instance;

	// build network
	NetworkBuilder network_builder;

	BackgroundSourcePopulation population_background_spike_source{
	    1,
	    {{HemisphereOnDLS(0), PADIBusOnPADIBusBlock(0)}},
	    BackgroundSourcePopulation::Config{
	        BackgroundSpikeSource::Period(period), BackgroundSpikeSource::Rate(),
	        BackgroundSpikeSource::Seed(), false}};
	auto const population_background_spike_source_descriptor =
	    network_builder.add(population_background_spike_source);

	Population::Neurons neurons{Population::Neuron(
	    LogicalNeuronOnDLS(
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	        AtomicNeuronOnDLS()),
	    Population::Neuron::Compartments{
	        {CompartmentOnLogicalNeuron(),
	         Population::Neuron::Compartment{
	             Population::Neuron::Compartment::SpikeMaster(0, true),
	             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})};
	Population population_internal{std::move(neurons)};
	auto const population_internal_descriptor = network_builder.add(population_internal);

	Projection::Connections projection_connections;
	for (size_t i = 0; i < population_background_spike_source.size; ++i) {
		projection_connections.push_back(
		    {{i, CompartmentOnLogicalNeuron()},
		     {i, CompartmentOnLogicalNeuron()},
		     Projection::Connection::Weight(63)});
	}
	Projection projection{
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), std::move(projection_connections),
	    population_background_spike_source_descriptor, population_internal_descriptor};
	network_builder.add(projection);

	auto const network = network_builder.done();

	// build network graph
	auto const routing_result = build_routing(network);
	auto const network_graph = build_network_graph(network, routing_result);

	// generate input
	grenade::vx::signal_flow::IODataMap inputs;
	inputs.runtime.push_back({{grenade::vx::signal_flow::ExecutionInstance(), running_period}});

	// run graph with given inputs and return results
	auto const result_map =
	    grenade::vx::execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

	auto const spikes = extract_neuron_spikes(result_map, network_graph);
	EXPECT_EQ(spikes.size(), 1);

	size_t expected_label_count =
	    spikes.at(0).contains({population_internal_descriptor, 0, CompartmentOnLogicalNeuron()})
	        ? spikes.at(0)
	              .at({population_internal_descriptor, 0, CompartmentOnLogicalNeuron()})
	              .size()
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
	auto chip_configs = get_chip_configs_bypass_excitatory();
	grenade::vx::execution::JITGraphExecutor executor;

	// 5% allowed deviation in spike count
	test_background_spike_source_regular(
	    BackgroundSpikeSource::Period(1000), grenade::vx::common::Time(1000000), 100, executor,
	    chip_configs);
	test_background_spike_source_regular(
	    BackgroundSpikeSource::Period(10000), grenade::vx::common::Time(10000000), 100, executor,
	    chip_configs);
}

void test_background_spike_source_poisson(
    BackgroundSpikeSource::Period period,
    BackgroundSpikeSource::Rate rate,
    grenade::vx::common::Time running_period,
    intmax_t spike_count_deviation,
    grenade::vx::execution::JITGraphExecutor& executor,
    grenade::vx::execution::JITGraphExecutor::ChipConfigs const& chip_configs)
{
	intmax_t expected_count = running_period * 2 /* f(FPGA) = 0.5 * f(BackgroundSpikeSource) */ /
	                          period / 64 /* population size */;

	typed_array<SpikeLabel, BackgroundSpikeSourceOnDLS> expected_labels;

	grenade::vx::signal_flow::ExecutionInstance instance;

	BackgroundSourcePopulation population_background_spike_source{
	    64,
	    {{HemisphereOnDLS(0), PADIBusOnPADIBusBlock(0)}},
	    BackgroundSourcePopulation::Config{
	        BackgroundSpikeSource::Period(period), BackgroundSpikeSource::Rate(rate),
	        BackgroundSpikeSource::Seed(1234), true}};

	Population::Neurons neurons;
	for (size_t i = 0; i < population_background_spike_source.size; ++i) {
		neurons.push_back(Population::Neuron(
		    LogicalNeuronOnDLS(
		        LogicalNeuronCompartments(
		            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
		        AtomicNeuronOnDLS(NeuronColumnOnDLS(i), NeuronRowOnDLS())),
		    Population::Neuron::Compartments{
		        {CompartmentOnLogicalNeuron(),
		         Population::Neuron::Compartment{
		             Population::Neuron::Compartment::SpikeMaster(0, true),
		             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}));
	}
	Population population_internal{std::move(neurons)};

	for (size_t i = 0; i < population_background_spike_source.size; ++i) {
		// build network
		NetworkBuilder network_builder;

		auto const population_background_spike_source_descriptor =
		    network_builder.add(population_background_spike_source);

		auto const population_internal_descriptor = network_builder.add(population_internal);

		Projection::Connections projection_connections;
		projection_connections.push_back(
		    {{i, CompartmentOnLogicalNeuron()},
		     {i, CompartmentOnLogicalNeuron()},
		     Projection::Connection::Weight(63)});

		Projection projection{
		    Receptor(Receptor::ID(), Receptor::Type::excitatory), std::move(projection_connections),
		    population_background_spike_source_descriptor, population_internal_descriptor};
		network_builder.add(projection);

		auto const network = network_builder.done();

		// build network graph
		auto const routing_result = build_routing(network);
		auto const network_graph = build_network_graph(network, routing_result);

		// generate input
		grenade::vx::signal_flow::IODataMap inputs;
		inputs.runtime.push_back({{grenade::vx::signal_flow::ExecutionInstance(), running_period}});

		// run graph with given inputs and return results
		auto const result_map =
		    grenade::vx::execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

		auto const spikes = extract_neuron_spikes(result_map, network_graph);
		EXPECT_EQ(spikes.size(), 1);

		std::array<intmax_t, 64> expected_label_counts;
		expected_label_counts.fill(0);
		for (auto [nrn, spike] : spikes.at(0)) {
			auto const& [descriptor, neuron_on_population, compartment_on_neuron] = nrn;
			auto const& neuron = std::get<Population>(network->populations.at(descriptor))
			                         .neurons.at(neuron_on_population);
			auto const an = neuron.coordinate.get_placed_compartments()
			                    .at(compartment_on_neuron)
			                    .at(neuron.compartments.at(compartment_on_neuron)
			                            .spike_master->neuron_on_compartment);
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
	auto chip_configs = get_chip_configs_bypass_excitatory();
	grenade::vx::execution::JITGraphExecutor executor;

	// 5% allowed deviation in spike count
	test_background_spike_source_poisson(
	    BackgroundSpikeSource::Period(1000), BackgroundSpikeSource::Rate(255),
	    grenade::vx::common::Time(1000000), 100, executor, chip_configs);
	test_background_spike_source_poisson(
	    BackgroundSpikeSource::Period(10000), BackgroundSpikeSource::Rate(255),
	    grenade::vx::common::Time(10000000), 100, executor, chip_configs);
}
