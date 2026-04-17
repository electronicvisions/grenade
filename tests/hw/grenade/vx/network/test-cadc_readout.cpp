#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/input_data.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/multi_index_sequence_dimension_unit/receptor_on_compartment.h"
#include "grenade/common/topology.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/abstract/calibration/fixture.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/mapper/greedy.h"
#include "grenade/vx/network/abstract/multi_index_sequence_dimension_unit/atomic_neuron_on_compartment.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/network/abstract/recorder/cadc.h"
#include "grenade/vx/network/receptor.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/background.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/systime.h"
#include "haldls/vx/v3/timer.h"
#include "helper.h"
#include "hxcomm/vx/connection_from_env.h"
#include "lola/vx/v3/chip.h"
#include <random>
#include <gtest/gtest.h>


using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace stadls::vx::v3;
using namespace lola::vx::v3;
using namespace haldls::vx::v3;
using namespace grenade::vx::network::abstract;
using namespace grenade::common;

TEST(CADCRecording, General)
{
	// Construct connection to HW
	auto chip = get_chip_config_bypass_excitatory();

	// CADC sampling shall take between one and {2.5, 7.0} us depending on placement of recording
	// data
	std::map<bool, float> expected_period = {{false, 2.5}, {true, 7.0}};

	for (auto const placement_on_dram : {false, true}) {
		grenade::vx::execution::JITGraphExecutor executor;

		// build topology
		auto topology = std::make_shared<Topology>();

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
		        {NeuronColumnOnDLS::size}, grenade::common::MultiIndex({0}),
		        {grenade::common::CellOnPopulationDimensionUnit()}),
		    UncalibratedNeuron::ParameterSpace(
		        NeuronColumnOnDLS::size, {{CompartmentOnNeuron(), 1}}),
		    grenade::common::TimeDomainOnTopology()};
		UncalibratedNeuron::ParameterSpace::Parameterization population_internal_input_data;
		population_internal_input_data.configs.resize(
		    NeuronColumnOnDLS::size, {{grenade::common::CompartmentOnNeuron(), {AtomicNeuron()}}});
		std::vector<size_t> all_internal_neurons;
		for (size_t i = 0; i < NeuronColumnOnDLS::size; ++i) {
			all_internal_neurons.push_back(i);
		}
		population_internal_input_data.base_configs.emplace_back(
		    all_internal_neurons, get_chip_config_bypass_excitatory());

		auto const population_internal_descriptor = topology->add_vertex(population_internal);

		CADCRecorder cadc_recorder(
		    CuboidMultiIndexSequence({4}), placement_on_dram, TimeDomainOnTopology());
		auto const cadc_recorder_descriptor = topology->add_vertex(cadc_recorder);

		topology->add_edge(
		    population_internal_descriptor, cadc_recorder_descriptor,
		    Edge(
		        ListMultiIndexSequence(
		            {MultiIndex({14, 0, 0}), MultiIndex({60, 0, 0}), MultiIndex({25, 0, 0}),
		             MultiIndex({150, 0, 0})},
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
		             AtomicNeuronOnCompartmentDimensionUnit()}),
		        CuboidMultiIndexSequence({4}), 1, 0));

		// build mapped topology
		FixtureCalibration const calibration;
		auto const mapped_topology =
		    std::make_shared<LinkedTopology>(GreedyMapper()(topology, calibration, executor));

		// generate input
		InputData input_data;
		ClockCycleTimeDomainRuntimes runtimes(
		    {grenade::vx::common::Time(
		         grenade::vx::common::Time::fpga_clock_cycles_per_us * 100 *
		         (placement_on_dram ? 10 : 1)),
		     grenade::vx::common::Time(
		         grenade::vx::common::Time::fpga_clock_cycles_per_us * 150 *
		         (placement_on_dram ? 10 : 1))},
		    grenade::vx::common::Time());
		input_data.time_domain_runtimes.set(TimeDomainOnTopology(), runtimes);

		input_data.ports.set({population_internal_descriptor, 1}, population_internal_input_data);

		auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

		// run topology with given inputs and return results
		auto const mapped_results =
		    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

		auto const results = mapped_topology->map_root_output_data(mapped_results);
		auto const result = dynamic_cast<CADCRecorder::Results const&>(
		                        results.ports.get({cadc_recorder_descriptor, 0}))
		                        .samples;

		EXPECT_EQ(result.size(), input_data.batch_size());
		std::set<grenade::vx::common::Time> unique_times;
		for (size_t i = 0; i < result.size(); ++i) {
			std::map<size_t, size_t> samples_per_neuron;
			auto const& samples = result.at(i);
			for (size_t j = 0; auto const& sample : samples) {
				for (auto const& [t, _] : sample) {
					unique_times.insert(t);
					samples_per_neuron[j] += 1;
				}
				j++;
			}
			EXPECT_EQ(samples_per_neuron.size(), cadc_recorder.get_shape().size())
			    << placement_on_dram;
			for (auto const& [_, num] : samples_per_neuron) {
				EXPECT_GE(
				    num, runtimes.values.at(i).value().value() /
				             Timer::Value::fpga_clock_cycles_per_us /
				             expected_period.at(placement_on_dram))
				    << placement_on_dram;
				EXPECT_LE(
				    num,
				    runtimes.values.at(i).value().value() / Timer::Value::fpga_clock_cycles_per_us)
				    << placement_on_dram;
			}
		}
		// only test that time is recorded and not all zeros
		EXPECT_GT(unique_times.size(), 1);
	}
}
