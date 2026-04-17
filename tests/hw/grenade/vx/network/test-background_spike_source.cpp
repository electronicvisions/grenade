#include <gtest/gtest.h>

#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/input_data.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/multi_index_sequence_dimension_unit/receptor_on_compartment.h"
#include "grenade/common/projection_connector/sequence.h"
#include "grenade/common/topology.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/abstract/calibration/fixture.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/mapper/greedy.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/recorder/spike.h"
#include "grenade/vx/network/receptor.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/background.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/systime.h"
#include "haldls/vx/v3/timer.h"
#include "helper.h"
#include "hxcomm/vx/connection_from_env.h"
#include "lola/vx/v3/chip.h"
#include <gtest/gtest.h>


using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace stadls::vx::v3;
using namespace lola::vx::v3;
using namespace haldls::vx::v3;
using namespace grenade::vx::network::abstract;
using namespace grenade::common;

void test_background_spike_source_poisson(
    BackgroundSpikeSource::Period period,
    BackgroundSpikeSource::Rate rate,
    grenade::vx::common::Time running_period,
    intmax_t spike_count_deviation,
    grenade::vx::execution::JITGraphExecutor& executor,
    bool record_directly)
{
	intmax_t expected_count = running_period * 2 /* f(FPGA) = 0.5 * f(BackgroundSpikeSource) */ /
	                          period / 64 /* population size */;

	Population population_background_spike_source{
	    PoissonSourceNeuron{},
	    CuboidMultiIndexSequence({64}, MultiIndex({0}), {CellOnPopulationDimensionUnit()}),
	    PoissonSourceNeuron::ParameterSpace(64), TimeDomainOnTopology()};

	PoissonSourceNeuron::ParameterSpace::Parameterization
	    population_background_spike_source_input_data(64);
	population_background_spike_source_input_data.period = BackgroundSpikeSource::Period(period);
	population_background_spike_source_input_data.rate = BackgroundSpikeSource::Rate(rate);
	population_background_spike_source_input_data.seed = BackgroundSpikeSource::Seed(1234);

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
	        {64}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    UncalibratedNeuron::ParameterSpace(64, {{CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};
	UncalibratedNeuron::ParameterSpace::Parameterization population_internal_input_data;
	population_internal_input_data.configs.resize(
	    64, {{grenade::common::CompartmentOnNeuron(), {AtomicNeuron()}}});
	std::vector<size_t> all_internal_neurons;
	for (size_t i = 0; i < 64; ++i) {
		all_internal_neurons.push_back(i);
	}
	population_internal_input_data.base_configs.emplace_back(
	    all_internal_neurons, get_chip_config_bypass_excitatory());

	SpikeRecorder spike_recorder(CuboidMultiIndexSequence({64}), TimeDomainOnTopology());

	for (size_t i = 0; i < population_background_spike_source.size(); ++i) {
		// build topology
		auto topology = std::make_shared<Topology>();

		auto const population_background_spike_source_descriptor =
		    topology->add_vertex(population_background_spike_source);

		auto const population_internal_descriptor = topology->add_vertex(population_internal);

		Projection projection(
		    UncalibratedSynapse{},
		    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
		    SequenceConnector{
		        CuboidMultiIndexSequence({64}), CuboidMultiIndexSequence({64}),
		        ListMultiIndexSequence({MultiIndex({i, i})})},
		    TimeDomainOnTopology());
		UncalibratedSynapse::ParameterSpace::Parameterization projection_input_data(
		    {UncalibratedSynapse::Weight(63)});

		auto const projection_descriptor = topology->add_vertex(projection);

		topology->add_edge(
		    population_background_spike_source_descriptor, projection_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {64, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({64}), 0, 0));

		topology->add_edge(
		    projection_descriptor, population_internal_descriptor,
		    Edge(
		        CuboidMultiIndexSequence({64}),
		        CuboidMultiIndexSequence(
		            {64, 1, 1}, MultiIndex({0, 0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
		             ReceptorOnCompartmentDimensionUnit()}),
		        0, 0));

		auto const spike_recorder_descriptor = topology->add_vertex(spike_recorder);

		topology->add_edge(
		    record_directly ? population_background_spike_source_descriptor
		                    : population_internal_descriptor,
		    spike_recorder_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {64, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({64}), 0, 0));

		// build mapped topology
		FixtureCalibration const calibration;
		auto const mapped_topology =
		    std::make_shared<LinkedTopology>(GreedyMapper()(topology, calibration, executor));

		// generate input
		InputData input_data;
		input_data.time_domain_runtimes.set(
		    TimeDomainOnTopology(),
		    ClockCycleTimeDomainRuntimes({running_period}, grenade::vx::common::Time()));

		input_data.ports.set(
		    {population_background_spike_source_descriptor, 0},
		    population_background_spike_source_input_data);
		input_data.ports.set({population_internal_descriptor, 1}, population_internal_input_data);
		input_data.ports.set({projection_descriptor, 1}, projection_input_data);

		auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

		// run topology with given inputs and return results
		auto const mapped_results =
		    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

		auto const results = mapped_topology->map_root_output_data(mapped_results);
		auto const spikes = dynamic_cast<SpikeRecorder::Results const&>(
		                        results.ports.get({spike_recorder_descriptor, 0}))
		                        .spikes;
		EXPECT_EQ(spikes.size(), 1);

		// check approx. equality in number of spikes expected
		size_t j = 0;
		for (auto const& neuron : spikes.at(0)) {
			intmax_t const neuron_spike_count = neuron.size();
			if (i == j) {
				EXPECT_TRUE(
				    (neuron_spike_count <= (expected_count + spike_count_deviation)) &&
				    (neuron_spike_count >= (expected_count - spike_count_deviation)))
				    << "expected: " << expected_count << " actual: " << neuron_spike_count
				    << "at: " << NeuronColumnOnDLS(i);
			} else {
				EXPECT_TRUE(
				    (neuron_spike_count <= (0 + spike_count_deviation)) &&
				    (neuron_spike_count >= (0 - spike_count_deviation)))
				    << "expected: " << 0 << " actual: " << neuron_spike_count
				    << "at: " << NeuronColumnOnDLS(i);
			}
			j++;
		}
	}
}

/**
 * Enable background sources and generate regular spike-trains.
 */
TEST(Model, BackgroundSpikeSourcePoisson)
{
	// Construct connection to HW
	grenade::vx::execution::JITGraphExecutor executor;

	// 5% allowed deviation in spike count
	test_background_spike_source_poisson(
	    BackgroundSpikeSource::Period(1000), BackgroundSpikeSource::Rate(255),
	    grenade::vx::common::Time(1000000), 100, executor, false);
	test_background_spike_source_poisson(
	    BackgroundSpikeSource::Period(10000), BackgroundSpikeSource::Rate(255),
	    grenade::vx::common::Time(10000000), 100, executor, false);
	test_background_spike_source_poisson(
	    BackgroundSpikeSource::Period(1000), BackgroundSpikeSource::Rate(255),
	    grenade::vx::common::Time(1000000), 100, executor, true);
	test_background_spike_source_poisson(
	    BackgroundSpikeSource::Period(10000), BackgroundSpikeSource::Rate(255),
	    grenade::vx::common::Time(10000000), 100, executor, true);
}
