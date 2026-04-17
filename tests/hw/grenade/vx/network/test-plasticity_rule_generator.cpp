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
#include "grenade/vx/network/abstract/plasticity_rule.h"
#include "grenade/vx/network/abstract/plasticity_rule_generator.h"
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


TEST(OnlyRecordingPlasticityRuleGenerator, weights)
{
	auto topology = std::make_shared<Topology>();

	// population at beginning of row
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
	        {1}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    UncalibratedNeuron::ParameterSpace(1, {{CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};
	UncalibratedNeuron::ParameterSpace::Parameterization population_internal_input_data;
	population_internal_input_data.configs.resize(
	    1, {{grenade::common::CompartmentOnNeuron(),
	         {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
	population_internal_input_data.base_configs.emplace_back(std::vector<size_t>{0}, chip);

	auto const population_descriptor_1 = topology->add_vertex(population_internal);

	// population not at beginning of row
	auto const population_descriptor_2 = topology->add_vertex(population_internal);

	// population on bottom row
	auto const population_descriptor_4 = topology->add_vertex(population_internal);

	// population on both rows
	Population population_internal_both{
	    UncalibratedNeuron{
	        UncalibratedNeuron::Compartments{
	            {grenade::common::CompartmentOnNeuron(),
	             UncalibratedNeuron::Compartment{
	                 UncalibratedNeuron::Compartment::SpikeMaster(0),
	                 {{{grenade::common::ReceptorOnCompartment(0),
	                    grenade::vx::network::Receptor::Type::excitatory},
	                   {grenade::common::ReceptorOnCompartment(0),
	                    grenade::vx::network::Receptor::Type::excitatory}}}}}},
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
	    grenade::common::CuboidMultiIndexSequence(
	        {2}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    UncalibratedNeuron::ParameterSpace(2, {{CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};
	UncalibratedNeuron::ParameterSpace::Parameterization population_internal_both_input_data;
	population_internal_both_input_data.configs.resize(
	    2, {{grenade::common::CompartmentOnNeuron(),
	         {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
	population_internal_both_input_data.base_configs.emplace_back(std::vector<size_t>{0, 1}, chip);

	auto const population_descriptor_5 = topology->add_vertex(population_internal_both);

	// projection at beginning of row
	Projection projection_1(
	    UncalibratedSynapse{},
	    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
	    SequenceConnector{
	        CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}),
	        ListMultiIndexSequence({MultiIndex({0, 0})})},
	    TimeDomainOnTopology());
	UncalibratedSynapse::ParameterSpace::Parameterization projection_1_input_data(
	    {UncalibratedSynapse::Weight(1)});

	auto const projection_descriptor_1 = topology->add_vertex(projection_1);

	topology->add_edge(
	    population_descriptor_1, projection_descriptor_1,
	    Edge(
	        CuboidMultiIndexSequence(
	            {1, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({1}), 0, 0));

	topology->add_edge(
	    projection_descriptor_1, population_descriptor_1,
	    Edge(
	        CuboidMultiIndexSequence({1}),
	        CuboidMultiIndexSequence(
	            {1, 1, 1}, MultiIndex({0, 0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
	             ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	// projection not at beginning of row
	UncalibratedSynapse::ParameterSpace::Parameterization projection_2_input_data(
	    {UncalibratedSynapse::Weight(2)});

	auto const projection_descriptor_2 = topology->add_vertex(projection_1);

	topology->add_edge(
	    population_descriptor_2, projection_descriptor_2,
	    Edge(
	        CuboidMultiIndexSequence(
	            {1, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({1}), 0, 0));

	topology->add_edge(
	    projection_descriptor_2, population_descriptor_2,
	    Edge(
	        CuboidMultiIndexSequence({1}),
	        CuboidMultiIndexSequence(
	            {1, 1, 1}, MultiIndex({0, 0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
	             ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	// projection not at beginning of column
	UncalibratedSynapse::ParameterSpace::Parameterization projection_3_input_data(
	    {UncalibratedSynapse::Weight(3)});

	auto const projection_descriptor_3 = topology->add_vertex(projection_1);

	topology->add_edge(
	    population_descriptor_1, projection_descriptor_3,
	    Edge(
	        CuboidMultiIndexSequence(
	            {1, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({1}), 0, 0));

	topology->add_edge(
	    projection_descriptor_3, population_descriptor_1,
	    Edge(
	        CuboidMultiIndexSequence({1}),
	        CuboidMultiIndexSequence(
	            {1, 1, 1}, MultiIndex({0, 0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
	             ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	// projection on bottom hemisphere
	UncalibratedSynapse::ParameterSpace::Parameterization projection_4_input_data(
	    {UncalibratedSynapse::Weight(4)});

	auto const projection_descriptor_4 = topology->add_vertex(projection_1);

	topology->add_edge(
	    population_descriptor_4, projection_descriptor_4,
	    Edge(
	        CuboidMultiIndexSequence(
	            {1, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({1}), 0, 0));

	topology->add_edge(
	    projection_descriptor_4, population_descriptor_4,
	    Edge(
	        CuboidMultiIndexSequence({1}),
	        CuboidMultiIndexSequence(
	            {1, 1, 1}, MultiIndex({0, 0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
	             ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	// projection over two hemispheres
	Projection projection_5(
	    UncalibratedSynapse{},
	    UncalibratedSynapse::ParameterSpace{
	        {UncalibratedSynapse::Weight(63), UncalibratedSynapse::Weight(63)}},
	    SequenceConnector{
	        CuboidMultiIndexSequence({2}), CuboidMultiIndexSequence({2}),
	        ListMultiIndexSequence({
	            MultiIndex({0, 0}),
	            MultiIndex({0, 1}),
	        })},
	    TimeDomainOnTopology());
	UncalibratedSynapse::ParameterSpace::Parameterization projection_5_input_data(
	    {UncalibratedSynapse::Weight(5), UncalibratedSynapse::Weight(5)});

	auto const projection_descriptor_5 = topology->add_vertex(projection_5);

	topology->add_edge(
	    population_descriptor_5, projection_descriptor_5,
	    Edge(
	        CuboidMultiIndexSequence(
	            {2, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({2}), 0, 0));

	topology->add_edge(
	    projection_descriptor_5, population_descriptor_5,
	    Edge(
	        CuboidMultiIndexSequence({2}),
	        CuboidMultiIndexSequence(
	            {2, 1, 1}, MultiIndex({0, 0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
	             ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	OnlyRecordingPlasticityRuleGenerator recording_generator(
	    {OnlyRecordingPlasticityRuleGenerator::Observable::weights});

	auto [timed_recording_config, plasticity_rule_parameterization] =
	    recording_generator.generate();

	std::vector<CuboidMultiIndexSequence> projection_shapes{
	    CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}),
	    CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({2})};

	PlasticityRule plasticity_rule(
	    timed_recording_config, PlasticityRule::ID(), {},
	    {projection_shapes.at(0), projection_shapes.at(1), projection_shapes.at(2),
	     projection_shapes.at(3), projection_shapes.at(4)},
	    TimeDomainOnTopology());

	PlasticityRule::Dynamics plasticity_rule_dynamics(
	    PlasticityRule::Dynamics::Timer{
	        PlasticityRule::Dynamics::Timer::Value(0),
	        PlasticityRule::Dynamics::Timer::Value(Timer::Value::fpga_clock_cycles_per_us * 300),
	        2},
	    1);

	auto const plasticity_rule_descriptor = topology->add_vertex(plasticity_rule);

	topology->add_edge(
	    projection_descriptor_1, plasticity_rule_descriptor,
	    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}), 1, 0));

	topology->add_edge(
	    projection_descriptor_2, plasticity_rule_descriptor,
	    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}), 1, 1));

	topology->add_edge(
	    projection_descriptor_3, plasticity_rule_descriptor,
	    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}), 1, 2));

	topology->add_edge(
	    projection_descriptor_4, plasticity_rule_descriptor,
	    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}), 1, 3));

	topology->add_edge(
	    projection_descriptor_5, plasticity_rule_descriptor,
	    Edge(CuboidMultiIndexSequence({2}), CuboidMultiIndexSequence({2}), 1, 4));

	// Construct connection to HW
	grenade::vx::execution::JITGraphExecutor executor;

	// build mapped topology
	FixtureCalibration const calibration;
	GreedyMapper mapper;
	mapper.set_neuron_permutation(
	    {AtomicNeuronOnDLS(NeuronColumnOnDLS(0), NeuronRowOnDLS(0)),
	     AtomicNeuronOnDLS(NeuronColumnOnDLS(1), NeuronRowOnDLS(0)),
	     AtomicNeuronOnDLS(NeuronColumnOnDLS(0), NeuronRowOnDLS(1)),
	     AtomicNeuronOnDLS(NeuronColumnOnDLS(2), NeuronRowOnDLS(0)),
	     AtomicNeuronOnDLS(NeuronColumnOnDLS(2), NeuronRowOnDLS(1))});

	auto const mapped_topology =
	    std::make_shared<LinkedTopology>(mapper(topology, calibration, executor));

	InputData input_data;
	input_data.time_domain_runtimes.set(
	    TimeDomainOnTopology(),
	    ClockCycleTimeDomainRuntimes(
	        std::vector(
	            1, std::optional<grenade::vx::common::Time>(
	                   grenade::vx::common::Time::fpga_clock_cycles_per_us * 1000)),
	        grenade::vx::common::Time()));

	input_data.ports.set({population_descriptor_1, 1}, population_internal_input_data);
	input_data.ports.set({population_descriptor_2, 1}, population_internal_input_data);
	input_data.ports.set({population_descriptor_4, 1}, population_internal_input_data);
	input_data.ports.set({population_descriptor_5, 1}, population_internal_both_input_data);

	input_data.ports.set({projection_descriptor_1, 1}, projection_1_input_data);
	input_data.ports.set({projection_descriptor_2, 1}, projection_2_input_data);
	input_data.ports.set({projection_descriptor_3, 1}, projection_3_input_data);
	input_data.ports.set({projection_descriptor_4, 1}, projection_4_input_data);
	input_data.ports.set({projection_descriptor_5, 1}, projection_5_input_data);

	input_data.ports.set({plasticity_rule_descriptor, 5}, plasticity_rule_parameterization);
	input_data.ports.set({plasticity_rule_descriptor, 6}, plasticity_rule_dynamics);

	auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

	// run graph with given inputs and return results
	auto const mapped_results =
	    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

	auto const results = mapped_topology->map_root_output_data(mapped_results);

	auto const recorded_data = std::get<PlasticityRule::Results::TimedData>(
	    dynamic_cast<PlasticityRule::Results const&>(
	        results.ports.get({plasticity_rule_descriptor, 0}))
	        .data);
	EXPECT_EQ(recorded_data.data_per_synapse.size(), 1);
	EXPECT_EQ(recorded_data.data_array.size(), 0);
	EXPECT_TRUE(recorded_data.data_per_synapse.contains("weights"));
	EXPECT_EQ(recorded_data.data_per_synapse.at("weights").size(), 5 /* #projections */);
	for (auto const& [descriptor, ws] : recorded_data.data_per_synapse.at("weights")) {
		auto const& weights = std::get<
		    std::vector<grenade::vx::common::TimedDataSequence<std::vector<std::vector<int8_t>>>>>(
		    ws);
		EXPECT_EQ(weights.size(), input_data.batch_size());
		auto const projection_descriptor =
		    std::vector{
		        projection_descriptor_1, projection_descriptor_2, projection_descriptor_3,
		        projection_descriptor_4, projection_descriptor_5}
		        .at(descriptor);
		for (size_t i = 0; i < weights.size(); ++i) {
			auto const& samples = weights.at(i);
			for (auto const& sample : samples) {
				if (projection_descriptor == projection_descriptor_5) {
					EXPECT_EQ(sample.data.size(), 2 /* #connections/projection */);
				} else {
					EXPECT_EQ(sample.data.size(), 1 /* #connections/projection */);
				}
				for (size_t i = 0; i < sample.data.size(); ++i) {
					EXPECT_EQ(
					    static_cast<int>(sample.data.at(i).at(0)),
					    dynamic_cast<UncalibratedSynapse::ParameterSpace::Parameterization const&>(
					        input_data.ports.get({projection_descriptor, 1}))
					        .weights.at(i));
				}
			}
		}
	}
}
