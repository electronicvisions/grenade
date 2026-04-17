#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/input_data.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/multi_index_sequence_dimension_unit/receptor_on_compartment.h"
#include "grenade/common/population.h"
#include "grenade/common/projection_connector/sequence.h"
#include "grenade/common/topology.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/abstract/calibration/fixture.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/mapper/greedy.h"
#include "grenade/vx/network/abstract/population_cell/delay.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/recorder/spike.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/background.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/systime.h"
#include "haldls/vx/v3/timer.h"
#include "helper.h"
#include "hxcomm/vx/connection_from_env.h"
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
using namespace grenade::vx::network::abstract;
using namespace grenade::common;


void test_external(grenade::vx::execution::JITGraphExecutor& executor)
{
	Population population_external{
	    ExternalSourceNeuron{}, CuboidMultiIndexSequence({2}, {CellOnPopulationDimensionUnit()}),
	    ExternalSourceNeuron::ParameterSpace(2), TimeDomainOnTopology()};

	SpikeRecorder spike_recorder(CuboidMultiIndexSequence({2}), TimeDomainOnTopology());

	// build network
	auto topology = std::make_shared<Topology>();

	auto const population_external_descriptor = topology->add_vertex(population_external);

	auto const spike_recorder_descriptor = topology->add_vertex(spike_recorder);

	topology->add_edge(
	    population_external_descriptor, spike_recorder_descriptor,
	    Edge(
	        CuboidMultiIndexSequence(
	            {2, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({2}), 0, 0));

	// build network graph
	FixtureCalibration const calibration;
	auto const mapped_topology =
	    std::make_shared<LinkedTopology>(GreedyMapper()(topology, calibration, executor));

	// generate input
	constexpr size_t num = 100;
	constexpr size_t isi = 1000;
	InputData input_data;
	std::vector<std::vector<std::vector<grenade::vx::common::Time>>> input_spike_times(
	    population_external.size());
	for (size_t b = 0; b < population_external.size(); ++b) {
		input_spike_times.at(b).resize(population_external.size());
		for (size_t j = 0; j < num; ++j) {
			input_spike_times.at(b).at(b).push_back(grenade::vx::common::Time(j * isi));
		}
	}
	input_data.ports.set(
	    {population_external_descriptor, 0}, ExternalSourceNeuron::Dynamics(input_spike_times));

	input_data.time_domain_runtimes.set(
	    TimeDomainOnTopology(),
	    ClockCycleTimeDomainRuntimes(
	        std::vector(
	            population_external.size(), std::optional<grenade::vx::common::Time>(num * isi)),
	        grenade::vx::common::Time()));

	auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

	// run graph with given inputs and return results
	auto const mapped_results =
	    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

	auto const results = mapped_topology->map_root_output_data(mapped_results);
	auto const spikes = dynamic_cast<SpikeRecorder::Results const&>(
	                        results.ports.get({spike_recorder_descriptor, 0}))
	                        .spikes;
	EXPECT_EQ(spikes.size(), population_external.size());

	for (size_t b = 0; b < population_external.size(); ++b) {
		for (size_t i = 0; i < population_external.size(); ++i) {
			if (i == b) {
				EXPECT_GE(spikes.at(b).at(i).size(), num * 0.8);
				EXPECT_LE(spikes.at(b).at(i).size(), num * 1.2);
			} else {
				EXPECT_TRUE(spikes.at(b).at(i).empty());
			}
		}
	}
}


void test_background(grenade::vx::execution::JITGraphExecutor& executor)
{
	grenade::vx::common::Time running_period(1000000);
	BackgroundSpikeSource::Period period(1000);
	BackgroundSpikeSource::Rate rate(255);
	size_t background_size = 2;
	intmax_t expected_count = running_period * 2 /* f(FPGA) = 0.5 * f(BackgroundSpikeSource) */ /
	                          period / background_size /* population size */;

	Population population_external{
	    PoissonSourceNeuron{},
	    CuboidMultiIndexSequence(
	        {background_size}, MultiIndex({0}), {CellOnPopulationDimensionUnit()}),
	    PoissonSourceNeuron::ParameterSpace(background_size), TimeDomainOnTopology()};

	PoissonSourceNeuron::ParameterSpace::Parameterization population_external_input_data(
	    background_size);
	population_external_input_data.period = BackgroundSpikeSource::Period(period);
	population_external_input_data.rate = BackgroundSpikeSource::Rate(rate);
	population_external_input_data.seed = BackgroundSpikeSource::Seed(1234);


	SpikeRecorder spike_recorder(
	    CuboidMultiIndexSequence({background_size}), TimeDomainOnTopology());

	// build network
	auto topology = std::make_shared<Topology>();

	auto const population_external_descriptor = topology->add_vertex(population_external);

	auto const spike_recorder_descriptor = topology->add_vertex(spike_recorder);

	topology->add_edge(
	    population_external_descriptor, spike_recorder_descriptor,
	    Edge(
	        CuboidMultiIndexSequence(
	            {background_size, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({background_size}), 0, 0));

	// build network graph
	FixtureCalibration const calibration;
	auto const mapped_topology =
	    std::make_shared<LinkedTopology>(GreedyMapper()(topology, calibration, executor));

	// generate input
	InputData input_data;
	input_data.time_domain_runtimes.set(
	    TimeDomainOnTopology(),
	    ClockCycleTimeDomainRuntimes({running_period}, grenade::vx::common::Time()));

	input_data.ports.set({population_external_descriptor, 0}, population_external_input_data);

	auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

	// run graph with given inputs and return results
	auto const mapped_results =
	    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

	auto const results = mapped_topology->map_root_output_data(mapped_results);
	auto const spikes = dynamic_cast<SpikeRecorder::Results const&>(
	                        results.ports.get({spike_recorder_descriptor, 0}))
	                        .spikes;
	EXPECT_EQ(spikes.size(), 1);

	// check approx. equality in number of spikes expected
	intmax_t spike_count_deviation = 100;
	size_t j = 0;
	for (auto const& neuron : spikes.at(0)) {
		intmax_t const neuron_spike_count = neuron.size();
		EXPECT_TRUE(
		    (neuron_spike_count <= (expected_count + spike_count_deviation)) &&
		    (neuron_spike_count >= (expected_count - spike_count_deviation)))
		    << "expected: " << expected_count << " actual: " << neuron_spike_count << "at: " << j;
		j++;
	}
}


void test_external_internal(
    grenade::vx::execution::JITGraphExecutor& executor, size_t internal_size)
{
	Population population_external{
	    ExternalSourceNeuron{}, CuboidMultiIndexSequence({2}, {CellOnPopulationDimensionUnit()}),
	    ExternalSourceNeuron::ParameterSpace(2), TimeDomainOnTopology()};

	auto const chip = get_chip_config_bypass_excitatory();
	Population population_internal{
	    UncalibratedNeuron{
	        UncalibratedNeuron::Compartments{
	            {grenade::common::CompartmentOnNeuron(),
	             UncalibratedNeuron::Compartment{
	                 UncalibratedNeuron::Compartment::SpikeMaster(0),
	                 {{{grenade::common::ReceptorOnCompartment(0),
	                    grenade::vx::network::Receptor::Type::excitatory}}}}}},
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
	    grenade::common::CuboidMultiIndexSequence(
	        {internal_size}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    UncalibratedNeuron::ParameterSpace(internal_size, {{CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};
	UncalibratedNeuron::ParameterSpace::Parameterization population_internal_input_data;
	population_internal_input_data.configs.resize(
	    internal_size, {{grenade::common::CompartmentOnNeuron(),
	                     {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
	std::vector<size_t> all_internal_neurons;
	for (size_t i = 0; i < internal_size; ++i) {
		all_internal_neurons.push_back(i);
	}
	population_internal_input_data.base_configs.emplace_back(all_internal_neurons, chip);

	SpikeRecorder spike_recorder(CuboidMultiIndexSequence({internal_size}), TimeDomainOnTopology());

	// build network
	for (auto i : {static_cast<size_t>(0), internal_size / 3, internal_size - 1}) {
		auto topology = std::make_shared<Topology>();

		auto const population_external_descriptor = topology->add_vertex(population_external);
		auto const population_internal_descriptor = topology->add_vertex(population_internal);

		auto const spike_recorder_descriptor = topology->add_vertex(spike_recorder);

		Projection projection(
		    UncalibratedSynapse{},
		    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
		    SequenceConnector{
		        CuboidMultiIndexSequence({2}), CuboidMultiIndexSequence({internal_size}),
		        ListMultiIndexSequence({MultiIndex({i % 2, i})})},
		    TimeDomainOnTopology());
		UncalibratedSynapse::ParameterSpace::Parameterization projection_input_data(
		    {UncalibratedSynapse::Weight(63)});

		auto const projection_descriptor = topology->add_vertex(projection);

		topology->add_edge(
		    population_external_descriptor, projection_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {2, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({2}), 0, 0));

		topology->add_edge(
		    projection_descriptor, population_internal_descriptor,
		    Edge(
		        CuboidMultiIndexSequence({internal_size}),
		        CuboidMultiIndexSequence(
		            {internal_size, 1, 1}, MultiIndex({0, 0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
		             ReceptorOnCompartmentDimensionUnit()}),
		        0, 0));

		topology->add_edge(
		    population_internal_descriptor, spike_recorder_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {internal_size, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({internal_size}), 0, 0));

		// build network graph
		FixtureCalibration const calibration;
		auto const mapped_topology =
		    std::make_shared<LinkedTopology>(GreedyMapper()(topology, calibration, executor));

		// generate input
		constexpr size_t num = 100;
		constexpr size_t isi = 1000;
		InputData input_data;
		std::vector<std::vector<std::vector<grenade::vx::common::Time>>> input_spike_times(1);
		input_spike_times.at(0).resize(population_external.size());
		for (size_t j = 0; j < num; ++j) {
			input_spike_times.at(0).at(i % 2).push_back(grenade::vx::common::Time(j * isi));
		}
		input_data.ports.set(
		    {population_external_descriptor, 0}, ExternalSourceNeuron::Dynamics(input_spike_times));
		input_data.ports.set({population_internal_descriptor, 1}, population_internal_input_data);
		input_data.ports.set({projection_descriptor, 1}, projection_input_data);

		input_data.time_domain_runtimes.set(
		    TimeDomainOnTopology(),
		    ClockCycleTimeDomainRuntimes(
		        {grenade::vx::common::Time(num * isi)}, grenade::vx::common::Time()));

		auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

		// run graph with given inputs and return results
		auto const mapped_results =
		    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

		auto const results = mapped_topology->map_root_output_data(mapped_results);
		auto const spikes = dynamic_cast<SpikeRecorder::Results const&>(
		                        results.ports.get({spike_recorder_descriptor, 0}))
		                        .spikes;
		EXPECT_EQ(spikes.size(), 1);

		for (size_t j = 0; j < population_external.size(); ++j) {
			if (i == j) {
				EXPECT_GE(spikes.at(0).at(j).size(), num * 0.8);
				EXPECT_LE(spikes.at(0).at(j).size(), num * 1.2);
			} else {
				EXPECT_TRUE(spikes.at(0).at(j).empty());
			}
		}
	}
}


void test_background_internal(
    grenade::vx::execution::JITGraphExecutor& executor, size_t internal_size)
{
	grenade::vx::common::Time running_period(1000000);
	BackgroundSpikeSource::Period period(1000);
	BackgroundSpikeSource::Rate rate(255);
	size_t background_size = 2;
	intmax_t expected_count = running_period * 2 /* f(FPGA) = 0.5 * f(BackgroundSpikeSource) */ /
	                          period / background_size /* population size */;

	Population population_external{
	    PoissonSourceNeuron{},
	    CuboidMultiIndexSequence(
	        {background_size}, MultiIndex({0}), {CellOnPopulationDimensionUnit()}),
	    PoissonSourceNeuron::ParameterSpace(background_size), TimeDomainOnTopology()};

	PoissonSourceNeuron::ParameterSpace::Parameterization population_external_input_data(
	    background_size);
	population_external_input_data.period = BackgroundSpikeSource::Period(period);
	population_external_input_data.rate = BackgroundSpikeSource::Rate(rate);
	population_external_input_data.seed = BackgroundSpikeSource::Seed(1234);

	auto const chip = get_chip_config_bypass_excitatory();
	Population population_internal{
	    UncalibratedNeuron{
	        UncalibratedNeuron::Compartments{
	            {grenade::common::CompartmentOnNeuron(),
	             UncalibratedNeuron::Compartment{
	                 UncalibratedNeuron::Compartment::SpikeMaster(0),
	                 {{{grenade::common::ReceptorOnCompartment(0),
	                    grenade::vx::network::Receptor::Type::excitatory}}}}}},
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
	    grenade::common::CuboidMultiIndexSequence(
	        {internal_size}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    UncalibratedNeuron::ParameterSpace(internal_size, {{CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};
	UncalibratedNeuron::ParameterSpace::Parameterization population_internal_input_data;
	population_internal_input_data.configs.resize(
	    internal_size, {{grenade::common::CompartmentOnNeuron(),
	                     {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
	std::vector<size_t> all_internal_neurons;
	for (size_t i = 0; i < internal_size; ++i) {
		all_internal_neurons.push_back(i);
	}
	population_internal_input_data.base_configs.emplace_back(all_internal_neurons, chip);

	SpikeRecorder spike_recorder(CuboidMultiIndexSequence({internal_size}), TimeDomainOnTopology());

	for (auto i : {static_cast<size_t>(0), internal_size / 3, internal_size - 1}) {
		// build network
		auto topology = std::make_shared<Topology>();

		auto const population_external_descriptor = topology->add_vertex(population_external);

		auto const population_internal_descriptor = topology->add_vertex(population_internal);

		Projection projection(
		    UncalibratedSynapse{},
		    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
		    SequenceConnector{
		        CuboidMultiIndexSequence({background_size}),
		        CuboidMultiIndexSequence({internal_size}),
		        ListMultiIndexSequence({MultiIndex({i % background_size, i})})},
		    TimeDomainOnTopology());
		UncalibratedSynapse::ParameterSpace::Parameterization projection_input_data(
		    {UncalibratedSynapse::Weight(63)});

		auto const projection_descriptor = topology->add_vertex(projection);

		topology->add_edge(
		    population_external_descriptor, projection_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {background_size, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({background_size}), 0, 0));

		topology->add_edge(
		    projection_descriptor, population_internal_descriptor,
		    Edge(
		        CuboidMultiIndexSequence({internal_size}),
		        CuboidMultiIndexSequence(
		            {internal_size, 1, 1}, MultiIndex({0, 0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
		             ReceptorOnCompartmentDimensionUnit()}),
		        0, 0));

		auto const spike_recorder_descriptor = topology->add_vertex(spike_recorder);

		topology->add_edge(
		    population_internal_descriptor, spike_recorder_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {internal_size, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({internal_size}), 0, 0));

		// build network graph
		FixtureCalibration const calibration;
		auto const mapped_topology =
		    std::make_shared<LinkedTopology>(GreedyMapper()(topology, calibration, executor));

		// generate input
		InputData input_data;
		input_data.time_domain_runtimes.set(
		    TimeDomainOnTopology(),
		    ClockCycleTimeDomainRuntimes({running_period}, grenade::vx::common::Time()));

		input_data.ports.set({population_external_descriptor, 0}, population_external_input_data);
		input_data.ports.set({population_internal_descriptor, 1}, population_internal_input_data);
		input_data.ports.set({projection_descriptor, 1}, projection_input_data);

		auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

		// run graph with given inputs and return results
		auto const mapped_results =
		    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

		auto const results = mapped_topology->map_root_output_data(mapped_results);
		auto const spikes = dynamic_cast<SpikeRecorder::Results const&>(
		                        results.ports.get({spike_recorder_descriptor, 0}))
		                        .spikes;
		EXPECT_EQ(spikes.size(), 1);

		// check approx. equality in number of spikes expected
		intmax_t spike_count_deviation = 100;
		size_t j = 0;
		for (auto const& neuron : spikes.at(0)) {
			intmax_t const neuron_spike_count = neuron.size();
			if (i == j) {
				EXPECT_TRUE(
				    (neuron_spike_count <= (expected_count + spike_count_deviation)) &&
				    (neuron_spike_count >= (expected_count - spike_count_deviation)))
				    << "expected: " << expected_count << " actual: " << neuron_spike_count
				    << "at: " << i;
			} else {
				EXPECT_TRUE(
				    (neuron_spike_count <= (0 + spike_count_deviation)) &&
				    (neuron_spike_count >= (0 - spike_count_deviation)))
				    << "expected: " << 0 << " actual: " << neuron_spike_count << "at: " << i;
			}
			j++;
		}
	}
}


void test_external_internal_internal(
    grenade::vx::execution::JITGraphExecutor& executor, size_t internal_size)
{
	Population population_external{
	    ExternalSourceNeuron{}, CuboidMultiIndexSequence({2}, {CellOnPopulationDimensionUnit()}),
	    ExternalSourceNeuron::ParameterSpace(2), TimeDomainOnTopology()};

	auto const chip = get_chip_config_bypass_excitatory();
	Population population_internal{
	    UncalibratedNeuron{
	        UncalibratedNeuron::Compartments{
	            {grenade::common::CompartmentOnNeuron(),
	             UncalibratedNeuron::Compartment{
	                 UncalibratedNeuron::Compartment::SpikeMaster(0),
	                 {{{grenade::common::ReceptorOnCompartment(0),
	                    grenade::vx::network::Receptor::Type::excitatory}}}}}},
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
	    grenade::common::CuboidMultiIndexSequence(
	        {internal_size}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    UncalibratedNeuron::ParameterSpace(internal_size, {{CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};
	UncalibratedNeuron::ParameterSpace::Parameterization population_internal_input_data;
	population_internal_input_data.configs.resize(
	    internal_size, {{grenade::common::CompartmentOnNeuron(),
	                     {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
	std::vector<size_t> all_internal_neurons;
	for (size_t i = 0; i < internal_size; ++i) {
		all_internal_neurons.push_back(i);
	}
	population_internal_input_data.base_configs.emplace_back(all_internal_neurons, chip);

	SpikeRecorder spike_recorder(CuboidMultiIndexSequence({internal_size}), TimeDomainOnTopology());

	// build network
	for (auto i : {static_cast<size_t>(0), internal_size / 3, internal_size - 1}) {
		auto topology = std::make_shared<Topology>();

		auto const population_external_descriptor = topology->add_vertex(population_external);
		auto const population_internal_0_descriptor = topology->add_vertex(population_internal);
		auto const population_internal_1_descriptor = topology->add_vertex(population_internal);

		auto const spike_recorder_descriptor = topology->add_vertex(spike_recorder);

		Projection projection_0(
		    UncalibratedSynapse{},
		    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
		    SequenceConnector{
		        CuboidMultiIndexSequence({2}), CuboidMultiIndexSequence({internal_size}),
		        ListMultiIndexSequence({MultiIndex({i % 2, i})})},
		    TimeDomainOnTopology());
		UncalibratedSynapse::ParameterSpace::Parameterization projection_0_input_data(
		    {UncalibratedSynapse::Weight(63)});

		Projection projection_1(
		    UncalibratedSynapse{},
		    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
		    SequenceConnector{
		        CuboidMultiIndexSequence({internal_size}),
		        CuboidMultiIndexSequence({internal_size}),
		        ListMultiIndexSequence({MultiIndex({i, i})})},
		    TimeDomainOnTopology());
		UncalibratedSynapse::ParameterSpace::Parameterization projection_1_input_data(
		    {UncalibratedSynapse::Weight(63)});

		auto const projection_0_descriptor = topology->add_vertex(projection_0);
		auto const projection_1_descriptor = topology->add_vertex(projection_1);

		topology->add_edge(
		    population_external_descriptor, projection_0_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {2, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({2}), 0, 0));

		topology->add_edge(
		    projection_0_descriptor, population_internal_0_descriptor,
		    Edge(
		        CuboidMultiIndexSequence({internal_size}),
		        CuboidMultiIndexSequence(
		            {internal_size, 1, 1}, MultiIndex({0, 0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
		             ReceptorOnCompartmentDimensionUnit()}),
		        0, 0));

		topology->add_edge(
		    population_internal_0_descriptor, projection_1_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {internal_size, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({internal_size}), 0, 0));

		topology->add_edge(
		    projection_1_descriptor, population_internal_1_descriptor,
		    Edge(
		        CuboidMultiIndexSequence({internal_size}),
		        CuboidMultiIndexSequence(
		            {internal_size, 1, 1}, MultiIndex({0, 0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
		             ReceptorOnCompartmentDimensionUnit()}),
		        0, 0));

		topology->add_edge(
		    population_internal_1_descriptor, spike_recorder_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {internal_size, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({internal_size}), 0, 0));

		// build network graph
		FixtureCalibration const calibration;
		auto const mapped_topology =
		    std::make_shared<LinkedTopology>(GreedyMapper()(topology, calibration, executor));

		// generate input
		constexpr size_t num = 100;
		constexpr size_t isi = 1000;
		InputData input_data;
		std::vector<std::vector<std::vector<grenade::vx::common::Time>>> input_spike_times(1);
		input_spike_times.at(0).resize(population_external.size());
		for (size_t j = 0; j < num; ++j) {
			input_spike_times.at(0).at(i % 2).push_back(grenade::vx::common::Time(j * isi));
		}
		input_data.ports.set(
		    {population_external_descriptor, 0}, ExternalSourceNeuron::Dynamics(input_spike_times));
		input_data.ports.set({population_internal_0_descriptor, 1}, population_internal_input_data);
		input_data.ports.set({population_internal_1_descriptor, 1}, population_internal_input_data);
		input_data.ports.set({projection_0_descriptor, 1}, projection_0_input_data);
		input_data.ports.set({projection_1_descriptor, 1}, projection_1_input_data);

		input_data.time_domain_runtimes.set(
		    TimeDomainOnTopology(),
		    ClockCycleTimeDomainRuntimes(
		        {grenade::vx::common::Time(num * isi)}, grenade::vx::common::Time()));

		auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

		// run graph with given inputs and return results
		auto const mapped_results =
		    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

		auto const results = mapped_topology->map_root_output_data(mapped_results);
		auto const spikes = dynamic_cast<SpikeRecorder::Results const&>(
		                        results.ports.get({spike_recorder_descriptor, 0}))
		                        .spikes;
		EXPECT_EQ(spikes.size(), 1);

		for (size_t j = 0; j < internal_size; ++j) {
			if (i == j) {
				EXPECT_GE(spikes.at(0).at(j).size(), num * 0.8);
				EXPECT_LE(spikes.at(0).at(j).size(), num * 1.2);
			} else {
				EXPECT_TRUE(spikes.at(0).at(j).empty());
			}
		}
	}
}

void test_external_internal_delay_internal(
    grenade::vx::execution::JITGraphExecutor& executor, size_t internal_size)
{
	Population population_external{
	    ExternalSourceNeuron{},
	    CuboidMultiIndexSequence({internal_size}, {CellOnPopulationDimensionUnit()}),
	    ExternalSourceNeuron::ParameterSpace(internal_size), TimeDomainOnTopology()};

	auto const chip = get_chip_config_bypass_excitatory();
	Population population_internal{
	    UncalibratedNeuron{
	        UncalibratedNeuron::Compartments{
	            {grenade::common::CompartmentOnNeuron(),
	             UncalibratedNeuron::Compartment{
	                 UncalibratedNeuron::Compartment::SpikeMaster(0),
	                 {{{grenade::common::ReceptorOnCompartment(0),
	                    grenade::vx::network::Receptor::Type::excitatory}}}}}},
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
	    grenade::common::CuboidMultiIndexSequence(
	        {internal_size}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    UncalibratedNeuron::ParameterSpace(internal_size, {{CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};
	UncalibratedNeuron::ParameterSpace::Parameterization population_internal_input_data;
	population_internal_input_data.configs.resize(
	    internal_size, {{grenade::common::CompartmentOnNeuron(),
	                     {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
	std::vector<size_t> all_internal_neurons;
	for (size_t i = 0; i < internal_size; ++i) {
		all_internal_neurons.push_back(i);
	}
	population_internal_input_data.base_configs.emplace_back(all_internal_neurons, chip);

	Population population_delay{
	    DelayCell{}, CuboidMultiIndexSequence({internal_size}, {CellOnPopulationDimensionUnit()}),
	    DelayCell::ParameterSpace(internal_size), TimeDomainOnTopology()};

	SpikeRecorder spike_recorder(CuboidMultiIndexSequence({internal_size}), TimeDomainOnTopology());

	// build network
	for (auto i : {static_cast<size_t>(0), internal_size / 3, internal_size - 1}) {
		auto topology = std::make_shared<Topology>();

		auto const population_external_descriptor = topology->add_vertex(population_external);
		auto const population_internal_0_descriptor = topology->add_vertex(population_internal);
		auto const population_delay_descriptor = topology->add_vertex(population_delay);
		auto const population_internal_1_descriptor = topology->add_vertex(population_internal);

		auto const spike_recorder_descriptor = topology->add_vertex(spike_recorder);

		Projection projection(
		    UncalibratedSynapse{},
		    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
		    SequenceConnector{
		        CuboidMultiIndexSequence({internal_size}),
		        CuboidMultiIndexSequence({internal_size}),
		        ListMultiIndexSequence({MultiIndex({i, i})})},
		    TimeDomainOnTopology());
		UncalibratedSynapse::ParameterSpace::Parameterization projection_input_data(
		    {UncalibratedSynapse::Weight(63)});

		auto const projection_0_descriptor = topology->add_vertex(projection);
		auto const projection_1_descriptor = topology->add_vertex(projection);

		topology->add_edge(
		    population_external_descriptor, projection_0_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {internal_size, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({internal_size}), 0, 0));

		topology->add_edge(
		    projection_0_descriptor, population_internal_0_descriptor,
		    Edge(
		        CuboidMultiIndexSequence({internal_size}),
		        CuboidMultiIndexSequence(
		            {internal_size, 1, 1}, MultiIndex({0, 0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
		             ReceptorOnCompartmentDimensionUnit()}),
		        0, 0));

		topology->add_edge(
		    population_internal_0_descriptor, population_delay_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {internal_size, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence(
		            {internal_size, 1},
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        0, 0));

		topology->add_edge(
		    population_delay_descriptor, projection_1_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {internal_size, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({internal_size}), 0, 0));

		topology->add_edge(
		    projection_1_descriptor, population_internal_1_descriptor,
		    Edge(
		        CuboidMultiIndexSequence({internal_size}),
		        CuboidMultiIndexSequence(
		            {internal_size, 1, 1}, MultiIndex({0, 0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
		             ReceptorOnCompartmentDimensionUnit()}),
		        0, 0));

		topology->add_edge(
		    population_internal_1_descriptor, spike_recorder_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {internal_size, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({internal_size}), 0, 0));

		// build network graph
		FixtureCalibration const calibration;
		auto const mapped_topology =
		    std::make_shared<LinkedTopology>(GreedyMapper()(topology, calibration, executor));

		// generate input
		constexpr size_t num = 100;
		constexpr size_t isi = 1000;
		InputData input_data;
		std::vector<std::vector<std::vector<grenade::vx::common::Time>>> input_spike_times(1);
		input_spike_times.at(0).resize(population_external.size());
		for (size_t j = 0; j < num; ++j) {
			input_spike_times.at(0).at(i).push_back(grenade::vx::common::Time(j * isi));
		}
		input_data.ports.set(
		    {population_external_descriptor, 0}, ExternalSourceNeuron::Dynamics(input_spike_times));
		input_data.ports.set({population_internal_0_descriptor, 1}, population_internal_input_data);
		input_data.ports.set({projection_0_descriptor, 1}, projection_input_data);
		input_data.ports.set({population_internal_1_descriptor, 1}, population_internal_input_data);
		input_data.ports.set({projection_1_descriptor, 1}, projection_input_data);
		input_data.ports.set(
		    {population_delay_descriptor, 1},
		    DelayCell::ParameterSpace::Parameterization(
		        std::vector(256, grenade::vx::common::Time(100))));

		input_data.time_domain_runtimes.set(
		    TimeDomainOnTopology(),
		    ClockCycleTimeDomainRuntimes(
		        {grenade::vx::common::Time(num * isi)}, grenade::vx::common::Time()));

		auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

		// run graph with given inputs and return results
		auto const mapped_results =
		    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

		auto const results = mapped_topology->map_root_output_data(mapped_results);
		auto const spikes = dynamic_cast<SpikeRecorder::Results const&>(
		                        results.ports.get({spike_recorder_descriptor, 0}))
		                        .spikes;
		EXPECT_EQ(spikes.size(), 1);

		for (size_t j = 0; j < internal_size; ++j) {
			if (i == j) {
				EXPECT_GE(spikes.at(0).at(j).size(), num * 0.8);
				EXPECT_LE(spikes.at(0).at(j).size(), num * 1.2);
			} else {
				EXPECT_TRUE(spikes.at(0).at(j).empty());
			}
		}
	}
}


TEST(Model, SingleChipExternal)
{
	grenade::vx::execution::JITGraphExecutor executor;
	test_external(executor);
}

TEST(Model, SingleChipBackground)
{
	grenade::vx::execution::JITGraphExecutor executor;
	test_background(executor);
}

TEST(Model, SingleChipExternalInternal)
{
	grenade::vx::execution::JITGraphExecutor executor;
	test_external_internal(executor, AtomicNeuronOnDLS::size);
}

TEST(Model, SingleChipBackgroundInternal)
{
	grenade::vx::execution::JITGraphExecutor executor;
	test_background_internal(executor, AtomicNeuronOnDLS::size);
}

TEST(Model, SingleChipExternalInternalInternal)
{
	grenade::vx::execution::JITGraphExecutor executor;
	test_external_internal_internal(executor, AtomicNeuronOnDLS::size / 2);
}

TEST(Model, MultiSingleChipExternalInternal)
{
	grenade::vx::execution::JITGraphExecutor executor;
	test_external_internal(executor, 2 * AtomicNeuronOnDLS::size);
}

TEST(Model, MultiSingleChipBackgroundInternal)
{
	grenade::vx::execution::JITGraphExecutor executor;
	test_background_internal(executor, 2 * AtomicNeuronOnDLS::size);
}

TEST(Model, MultiSingleChipExternalInternalInternal)
{
	grenade::vx::execution::JITGraphExecutor executor;
	test_external_internal_internal(executor, AtomicNeuronOnDLS::size);
}

TEST(Model, ExternalInternalDelayInternal)
{
	grenade::vx::execution::JITGraphExecutor executor;
	test_external_internal_delay_internal(executor, AtomicNeuronOnDLS::size / 2);
}
