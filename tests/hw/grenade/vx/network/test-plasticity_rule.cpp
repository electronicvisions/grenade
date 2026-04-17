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
#include "grenade/vx/network/abstract/execution_instance_global.h"
#include "grenade/vx/network/abstract/mapper/greedy.h"
#include "grenade/vx/network/abstract/multi_index_sequence_dimension_unit/atomic_neuron_on_compartment.h"
#include "grenade/vx/network/abstract/plasticity_rule.h"
#include "grenade/vx/network/abstract/plasticity_rule_generator.h"
#include "grenade/vx/network/abstract/population_cell/delay.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/recorder/spike.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/background.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/systime.h"
#include "haldls/vx/v3/timer.h"
#include "hate/variant.h"
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

TEST(PlasticityRule, RawRecording)
{
	// build network
	auto topology = std::make_shared<Topology>();

	auto const chip = get_chip_config_bypass_excitatory();
	Population population{
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
	UncalibratedNeuron::ParameterSpace::Parameterization population_input_data;
	population_input_data.configs.resize(
	    1, {{grenade::common::CompartmentOnNeuron(),
	         {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
	population_input_data.base_configs.emplace_back(std::vector<size_t>{0}, chip);

	auto const population_descriptor = topology->add_vertex(population);

	Projection projection(
	    UncalibratedSynapse{},
	    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
	    SequenceConnector{
	        CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}),
	        ListMultiIndexSequence({MultiIndex({0, 0})})},
	    TimeDomainOnTopology());
	UncalibratedSynapse::ParameterSpace::Parameterization projection_input_data(
	    {UncalibratedSynapse::Weight(1)});

	auto const projection_descriptor = topology->add_vertex(projection);

	topology->add_edge(
	    population_descriptor, projection_descriptor,
	    Edge(
	        CuboidMultiIndexSequence(
	            {1, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({1}), 0, 0));

	topology->add_edge(
	    projection_descriptor, population_descriptor,
	    Edge(
	        CuboidMultiIndexSequence({1}),
	        CuboidMultiIndexSequence(
	            {1, 1, 1}, MultiIndex({0, 0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
	             ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	std::vector<CuboidMultiIndexSequence> projection_shapes{CuboidMultiIndexSequence({1})};

	PlasticityRule plasticity_rule(
	    PlasticityRule::RawRecording{8, true}, PlasticityRule::ID(), {}, {projection_shapes.at(0)},
	    TimeDomainOnTopology());

	PlasticityRule::Dynamics plasticity_rule_dynamics(
	    PlasticityRule::Dynamics::Timer{
	        PlasticityRule::Dynamics::Timer::Value(1000),
	        PlasticityRule::Dynamics::Timer::Value(10000), 1},
	    2);

	std::stringstream kernel;
	kernel << "#include \"grenade/vx/ppu/synapse_array_view_handle.h\"\n";
	kernel << "#include \"grenade/vx/ppu/neuron_view_handle.h\"\n";
	kernel << "#include \"libnux/vx/location.h\"\n";
	kernel << "using namespace grenade::vx::ppu;\n";
	kernel << "using namespace libnux::vx;\n";
	kernel << "void PLASTICITY_RULE_KERNEL(std::array<SynapseArrayViewHandle, 1>& synapses, "
	          "std::array<NeuronViewHandle, 0>& neurons, "
	          "Recording& recording)\n";
	kernel << "{\n";
	kernel << "  for (size_t i = 0; i < recording.memory.size(); ++i) {\n";
	kernel << "    recording.memory[i] = i;\n";
	kernel << "  }\n";
	kernel << "}\n";

	PlasticityRule::Parameterization plasticity_rule_parameterization(kernel.str());

	auto const plasticity_rule_descriptor = topology->add_vertex(plasticity_rule);

	topology->add_edge(
	    projection_descriptor, plasticity_rule_descriptor,
	    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}), 1, 0));

	// Construct connection to HW
	grenade::vx::execution::JITGraphExecutor executor;

	// build network graph
	FixtureCalibration const calibration;
	GreedyMapper mapper;
	auto const mapped_topology =
	    std::make_shared<LinkedTopology>(mapper(topology, calibration, executor));

	InputData input_data;
	input_data.time_domain_runtimes.set(
	    TimeDomainOnTopology(),
	    ClockCycleTimeDomainRuntimes(
	        {std::optional<grenade::vx::common::Time>(
	             grenade::vx::common::Time::fpga_clock_cycles_per_us * 1000),
	         std::optional<grenade::vx::common::Time>(
	             grenade::vx::common::Time::fpga_clock_cycles_per_us * 1000)},
	        grenade::vx::common::Time()));

	input_data.ports.set({population_descriptor, 1}, population_input_data);
	input_data.ports.set({projection_descriptor, 1}, projection_input_data);
	input_data.ports.set({plasticity_rule_descriptor, 1}, plasticity_rule_parameterization);
	input_data.ports.set({plasticity_rule_descriptor, 2}, plasticity_rule_dynamics);

	auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

	// run graph with given inputs and return results
	auto const mapped_results =
	    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

	auto const results = mapped_topology->map_root_output_data(mapped_results);

	auto const result = std::get<PlasticityRule::Results::RawData>(
	                        dynamic_cast<PlasticityRule::Results const&>(
	                            results.ports.get({plasticity_rule_descriptor, 0}))
	                            .data)
	                        .data;

	EXPECT_EQ(result.size(), input_data.batch_size());
	for (size_t i = 0; i < result.size(); ++i) {
		auto const& samples = result.at(i);
		EXPECT_EQ(samples.size(), 8);
		for (size_t j = 0; auto const& sample : samples) {
			EXPECT_EQ(static_cast<int>(sample), j % 8);
			j++;
		}
	}
}

TEST(PlasticityRule, TimedRecordingConfig)
{
	auto const chip = get_chip_config_bypass_excitatory();

	InputData input_data;
	input_data.time_domain_runtimes.set(
	    TimeDomainOnTopology(),
	    ClockCycleTimeDomainRuntimes(
	        {std::optional<grenade::vx::common::Time>(
	             grenade::vx::common::Time::fpga_clock_cycles_per_us * 10000),
	         std::optional<grenade::vx::common::Time>(
	             grenade::vx::common::Time::fpga_clock_cycles_per_us * 10000)},
	        grenade::vx::common::Time()));

	grenade::vx::execution::JITGraphExecutor executor;

	std::mt19937 rng(std::random_device{}());
	constexpr size_t num = 10;
	for (size_t n = 0; n < num; ++n) {
		// build network
		auto topology = std::make_shared<Topology>();

		// FIXME: Fix is required in order in routing
		std::uniform_int_distribution num_projection_distribution(1, 3);
		size_t const num_projections = num_projection_distribution(rng);
		std::vector<VertexOnTopology> projection_descriptors;
		std::vector<VertexOnTopology> population_descriptors;
		std::vector<std::reference_wrapper<MultiIndexSequence const>> projection_shapes;
		std::list<CuboidMultiIndexSequence> projection_shapes_storage;
		std::vector<std::reference_wrapper<MultiIndexSequence const>> population_shapes;
		for (size_t p = 0; p < num_projections; ++p) {
			std::uniform_int_distribution neuron_distribution(1, 3);

			// Adding source population
			size_t const num_neurons_s = neuron_distribution(rng);
			std::vector<size_t> neuron_indices_s;
			for (size_t n = 0; n < num_neurons_s; ++n) {
				neuron_indices_s.push_back(n);
			}
			Population population_s{
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
			        {num_neurons_s}, grenade::common::MultiIndex({0}),
			        {grenade::common::CellOnPopulationDimensionUnit()}),
			    UncalibratedNeuron::ParameterSpace(num_neurons_s, {{CompartmentOnNeuron(), 1}}),
			    grenade::common::TimeDomainOnTopology()};
			UncalibratedNeuron::ParameterSpace::Parameterization population_s_input_data;
			population_s_input_data.configs.resize(
			    num_neurons_s, {{grenade::common::CompartmentOnNeuron(),
			                     {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
			population_s_input_data.base_configs.emplace_back(neuron_indices_s, chip);

			// Adding target population
			size_t const num_neurons_t = neuron_distribution(rng);
			std::vector<size_t> neuron_indices_t;
			for (size_t n = 0; n < num_neurons_t; ++n) {
				neuron_indices_t.push_back(n);
			}
			Population population_t{
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
			        {num_neurons_t}, grenade::common::MultiIndex({0}),
			        {grenade::common::CellOnPopulationDimensionUnit()}),
			    UncalibratedNeuron::ParameterSpace(num_neurons_t, {{CompartmentOnNeuron(), 1}}),
			    grenade::common::TimeDomainOnTopology()};
			UncalibratedNeuron::ParameterSpace::Parameterization population_t_input_data;
			population_t_input_data.configs.resize(
			    num_neurons_t, {{grenade::common::CompartmentOnNeuron(),
			                     {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
			population_t_input_data.base_configs.emplace_back(neuron_indices_t, chip);

			auto const population_descriptor_s = topology->add_vertex(population_s);
			auto const population_descriptor_t = topology->add_vertex(population_t);

			Projection projection(
			    UncalibratedSynapse{},
			    UncalibratedSynapse::ParameterSpace{std::vector(
			        population_s.size() * population_t.size(), UncalibratedSynapse::Weight(63))},
			    SequenceConnector{
			        CuboidMultiIndexSequence({population_s.size()}),
			        CuboidMultiIndexSequence({population_t.size()}),
			        CuboidMultiIndexSequence({population_s.size(), population_t.size()})},
			    TimeDomainOnTopology());
			UncalibratedSynapse::ParameterSpace::Parameterization projection_input_data(std::vector(
			    population_s.size() * population_t.size(), UncalibratedSynapse::Weight(1)));

			auto const projection_descriptor = topology->add_vertex(projection);

			topology->add_edge(
			    population_descriptor_s, projection_descriptor,
			    Edge(
			        CuboidMultiIndexSequence(
			            {population_s.size(), 1}, MultiIndex({0, 0}),
			            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
			        CuboidMultiIndexSequence({population_s.size()}), 0, 0));

			topology->add_edge(
			    projection_descriptor, population_descriptor_t,
			    Edge(
			        CuboidMultiIndexSequence({population_t.size()}),
			        CuboidMultiIndexSequence(
			            {population_t.size(), 1, 1}, MultiIndex({0, 0, 0}),
			            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
			             ReceptorOnCompartmentDimensionUnit()}),
			        0, 0));

			projection_descriptors.push_back(projection_descriptor);
			projection_shapes_storage.push_back(
			    CuboidMultiIndexSequence({population_s.size(), population_t.size()}));
			projection_shapes.push_back(projection_shapes_storage.back());
			population_shapes.push_back(
			    dynamic_cast<Population const&>(topology->get(population_descriptor_t))
			        .get_shape());
			population_descriptors.push_back(population_descriptor_t);

			input_data.ports.set({population_descriptor_s, 1}, population_s_input_data);
			input_data.ports.set({population_descriptor_t, 1}, population_t_input_data);
			input_data.ports.set({projection_descriptor, 1}, projection_input_data);
		}

		std::uniform_int_distribution num_periods_distribution(1, 3);
		PlasticityRule::Dynamics plasticity_rule_dynamics(
		    PlasticityRule::Dynamics::Timer{
		        PlasticityRule::Dynamics::Timer::Value(0),
		        PlasticityRule::Dynamics::Timer::Value(
		            grenade::vx::common::Time::fpga_clock_cycles_per_us * 3000),
		        static_cast<size_t>(num_periods_distribution(rng))},
		    2);

		std::stringstream kernel;
		kernel << "#include \"grenade/vx/ppu/synapse_array_view_handle.h\"\n";
		kernel << "#include \"grenade/vx/ppu/neuron_view_handle.h\"\n";
		kernel << "#include \"libnux/vx/location.h\"\n";
		kernel << "#include \"hate/tuple.h\"\n";
		kernel << "using namespace grenade::vx::ppu;\n";
		kernel << "using namespace libnux::vx;\n";
		kernel << "template <size_t N, size_t M>\n";
		kernel << "void PLASTICITY_RULE_KERNEL(std::array<SynapseArrayViewHandle, N>&, "
		          "std::array<NeuronViewHandle, M>&, Recording& "
		          "recording)\n";
		kernel << "{\n";
		kernel << "\tstatic size_t period = 0;\n";

		// generate random timed recording
		PlasticityRule::TimedRecordingConfig recording;
		// select number of observables
		std::uniform_int_distribution num_observable_distribution(1, 3);
		size_t const num_observables = num_observable_distribution(rng);
		std::vector<intmax_t> expectations;
		// generate observables
		for (size_t i = 0; i < num_observables; ++i) {
			// select which type of observable to generate
			// 0: ObservableArray, 1: ObservablePerSynapse, 2: ObservablePerNeuron
			switch (std::uniform_int_distribution(0, 2)(rng)) {
				case 0: { // ObservableArray
					// select observable size
					auto obsv = PlasticityRule::TimedRecordingConfig::ObservableArray{
					    PlasticityRule::TimedRecordingConfig::ObservableArray::Type::int8,
					    std::uniform_int_distribution<size_t>(1, 15)(rng)};
					// select data type
					switch (std::uniform_int_distribution{0, 3}(rng)) {
						case 0: { // int8
							obsv.type =
							    PlasticityRule::TimedRecordingConfig::ObservableArray::Type::int8;
							expectations.push_back(std::uniform_int_distribution<int8_t>()(rng));
							break;
						}
						case 1: { // uint8
							obsv.type =
							    PlasticityRule::TimedRecordingConfig::ObservableArray::Type::uint8;
							expectations.push_back(std::uniform_int_distribution<uint8_t>()(rng));
							break;
						}
						case 2: { // int16
							obsv.type =
							    PlasticityRule::TimedRecordingConfig::ObservableArray::Type::int16;
							expectations.push_back(std::uniform_int_distribution<int16_t>()(rng));
							break;
						}
						case 3: { // uint16
							obsv.type =
							    PlasticityRule::TimedRecordingConfig::ObservableArray::Type::uint16;
							expectations.push_back(std::uniform_int_distribution<uint16_t>()(rng));
							break;
						}
						default: {
							throw std::logic_error("Unknown observable type");
						}
					}
					recording.observables["observable_" + std::to_string(i)] = obsv;
					// set its value from the PPU to expectation
					kernel << "\trecording.observable_" << i << ".fill("
					       << expectations.at(expectations.size() - 1) << ");\n";
					break;
				}
				case 1: { // ObservablePerSynapse
					// select memory layout per row
					auto obsv = PlasticityRule::TimedRecordingConfig::ObservablePerSynapse{
					    PlasticityRule::TimedRecordingConfig::ObservablePerSynapse::Type::int8,
					    std::bernoulli_distribution{}(rng)
					        ? PlasticityRule::TimedRecordingConfig::ObservablePerSynapse::
					              LayoutPerRow::complete_rows
					        : PlasticityRule::TimedRecordingConfig::ObservablePerSynapse::
					              LayoutPerRow::packed_active_columns};
					// select data type
					switch (std::uniform_int_distribution{0, 3}(rng)) {
						case 0: { // int8
							obsv.type = PlasticityRule::TimedRecordingConfig::ObservablePerSynapse::
							    Type::int8;
							expectations.push_back(std::uniform_int_distribution<int8_t>()(rng));
							break;
						}
						case 1: { // uint8
							obsv.type = PlasticityRule::TimedRecordingConfig::ObservablePerSynapse::
							    Type::uint8;
							expectations.push_back(std::uniform_int_distribution<uint8_t>()(rng));
							break;
						}
						case 2: { // int16
							obsv.type = PlasticityRule::TimedRecordingConfig::ObservablePerSynapse::
							    Type::int16;
							expectations.push_back(std::uniform_int_distribution<int16_t>()(rng));
							break;
						}
						case 3: { // uint16
							obsv.type = PlasticityRule::TimedRecordingConfig::ObservablePerSynapse::
							    Type::uint16;
							expectations.push_back(std::uniform_int_distribution<uint16_t>()(rng));
							break;
						}
						default: {
							throw std::logic_error("Unknown observable type");
						}
					}
					recording.observables["observable_" + std::to_string(i)] = obsv;
					// set its value from the PPU to i
					kernel << "\thate::for_each([](auto& rec) { ";
					switch (obsv.layout_per_row) {
						case PlasticityRule::TimedRecordingConfig::ObservablePerSynapse::
						    LayoutPerRow::complete_rows: {
							kernel << "for (auto& row: rec) { row = "
							       << expectations.at(expectations.size() - 1) << "; } },\n";
							break;
						}
						case PlasticityRule::TimedRecordingConfig::ObservablePerSynapse::
						    LayoutPerRow::packed_active_columns: {
							kernel << "for (auto& row: rec) { row.fill("
							       << expectations.at(expectations.size() - 1) << "); } },\n";
							break;
						}
						default: {
							throw std::logic_error("Layout per row unknown.");
						}
					}
					kernel << "\trecording.observable_" << i << ");\n";
					break;
				}
				case 2: { // ObservablePerNeuron
					// select memory layout
					auto obsv = PlasticityRule::TimedRecordingConfig::ObservablePerNeuron{
					    PlasticityRule::TimedRecordingConfig::ObservablePerNeuron::Type::int8,
					    std::bernoulli_distribution{}(rng)
					        ? PlasticityRule::TimedRecordingConfig::ObservablePerNeuron::Layout::
					              complete_row
					        : PlasticityRule::TimedRecordingConfig::ObservablePerNeuron::Layout::
					              packed_active_columns};
					// select data type
					switch (std::uniform_int_distribution{0, 3}(rng)) {
						case 0: { // int8
							obsv.type = PlasticityRule::TimedRecordingConfig::ObservablePerNeuron::
							    Type::int8;
							expectations.push_back(std::uniform_int_distribution<int8_t>()(rng));
							break;
						}
						case 1: { // uint8
							obsv.type = PlasticityRule::TimedRecordingConfig::ObservablePerNeuron::
							    Type::uint8;
							expectations.push_back(std::uniform_int_distribution<uint8_t>()(rng));
							break;
						}
						case 2: { // int16
							obsv.type = PlasticityRule::TimedRecordingConfig::ObservablePerNeuron::
							    Type::int16;
							expectations.push_back(std::uniform_int_distribution<int16_t>()(rng));
							break;
						}
						case 3: { // uint16
							obsv.type = PlasticityRule::TimedRecordingConfig::ObservablePerNeuron::
							    Type::uint16;
							expectations.push_back(std::uniform_int_distribution<uint16_t>()(rng));
							break;
						}
						default: {
							throw std::logic_error("Unknown observable type");
						}
					}
					recording.observables["observable_" + std::to_string(i)] = obsv;
					// set its value from the PPU to i
					kernel << "\thate::for_each([](auto& rec) { ";
					switch (obsv.layout) {
						case PlasticityRule::TimedRecordingConfig::ObservablePerNeuron::Layout::
						    complete_row: {
							kernel << "rec = " << expectations.at(expectations.size() - 1) << ";\n";
							break;
						}
						case PlasticityRule::TimedRecordingConfig::ObservablePerNeuron::Layout::
						    packed_active_columns: {
							kernel << "rec.fill(" << expectations.at(expectations.size() - 1)
							       << ");\n";
							break;
						}
						default: {
							throw std::logic_error("Layout unknown.");
						}
					}
					kernel << "\t}, recording.observable_" << i << ");\n";
					break;
				}
				default: {
					throw std::logic_error("Unknown observable case.");
				}
			}
		}

		PlasticityRule plasticity_rule(
		    recording, PlasticityRule::ID(), population_shapes, projection_shapes,
		    TimeDomainOnTopology());

		kernel << "\trecording.time = period * 2 /* f_ppu = 2 * f_fpga */;\n";
		kernel << "\tperiod++;\n";
		kernel << "\tperiod %= " << plasticity_rule_dynamics.timer.num_periods << ";\n";
		kernel << "}";

		PlasticityRule::Parameterization plasticity_rule_parameterization(kernel.str());

		auto const plasticity_rule_descriptor = topology->add_vertex(plasticity_rule);

		for (size_t p = 0; p < projection_descriptors.size(); ++p) {
			topology->add_edge(
			    projection_descriptors.at(p), plasticity_rule_descriptor,
			    Edge(
			        CuboidMultiIndexSequence({projection_shapes.at(p).get().size()}),
			        projection_shapes.at(p).get(), 1, p));
		}

		for (size_t p = 0; p < population_descriptors.size(); ++p) {
			topology->add_edge(
			    population_descriptors.at(p), plasticity_rule_descriptor,
			    Edge(
			        CuboidMultiIndexSequence(
			            {population_shapes.at(p).get().size(), 1, 1},
			            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
			             AtomicNeuronOnCompartmentDimensionUnit()}),
			        population_shapes.at(p).get(), 1, projection_descriptors.size() + p));
		}

		input_data.ports.set(
		    {plasticity_rule_descriptor, population_shapes.size() + projection_shapes.size()},
		    plasticity_rule_parameterization);
		input_data.ports.set(
		    {plasticity_rule_descriptor, population_shapes.size() + projection_shapes.size() + 1},
		    plasticity_rule_dynamics);

		// build mapped topology
		FixtureCalibration const calibration;
		GreedyMapper mapper;
		auto const mapped_topology =
		    std::make_shared<LinkedTopology>(mapper(topology, calibration, executor));

		auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

		// run graph with given inputs and return results
		auto const mapped_results =
		    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

		auto const results = mapped_topology->map_root_output_data(mapped_results);

		auto const timed_recording_data = std::get<PlasticityRule::Results::TimedData>(
		    dynamic_cast<PlasticityRule::Results const&>(
		        results.ports.get({plasticity_rule_descriptor, 0}))
		        .data);

		for (size_t j = 0; auto const& [name, observable] : recording.observables) {
			std::visit(
			    hate::overloaded{
			        [&](PlasticityRule::TimedRecordingConfig::ObservableArray const& obsv) {
				        EXPECT_TRUE(timed_recording_data.data_array.contains(name));
				        auto const& data_entry_variant = timed_recording_data.data_array.at(name);
				        std::visit(
				            [&](auto type) {
					            auto const& data_entry =
					                std::get<std::vector<grenade::vx::common::TimedDataSequence<
					                    std::vector<typename decltype(type)::ElementType>>>>(
					                    data_entry_variant);
					            EXPECT_EQ(data_entry.size(), input_data.batch_size());
					            for (size_t i = 0; i < data_entry.size(); ++i) {
						            auto const& samples = data_entry.at(i);
						            EXPECT_EQ(
						                samples.size(), plasticity_rule_dynamics.timer.num_periods);
						            for (size_t time = 0; auto const& sample : samples) {
							            EXPECT_EQ(sample.time, time);
							            EXPECT_EQ(sample.data.size(), obsv.size);
							            for (size_t i = 0; i < sample.data.size(); ++i) {
								            EXPECT_EQ(
								                static_cast<int>(sample.data.at(i)),
								                expectations.at(j))
								                << name;
							            }
							            time++;
						            }
					            }
				            },
				            obsv.type);
			        },
			        [&](PlasticityRule::TimedRecordingConfig::ObservablePerSynapse const& obsv) {
				        EXPECT_TRUE(timed_recording_data.data_per_synapse.contains(name));
				        auto const& data_entry_variant =
				            timed_recording_data.data_per_synapse.at(name);
				        std::visit(
				            [&](auto type) {
					            EXPECT_EQ(data_entry_variant.size(), projection_shapes.size());
					            for (size_t p = 0; p < projection_shapes.size(); ++p) {
						            auto const& data_entry = std::get<std::vector<
						                grenade::vx::common::TimedDataSequence<std::vector<
						                    std::vector<typename decltype(type)::ElementType>>>>>(
						                data_entry_variant.at(p));
						            EXPECT_EQ(data_entry.size(), input_data.batch_size());
						            for (size_t i = 0; i < data_entry.size(); ++i) {
							            auto const& samples = data_entry.at(i);
							            EXPECT_EQ(
							                samples.size(),
							                plasticity_rule_dynamics.timer.num_periods);
							            for (size_t time = 0; auto const& sample : samples) {
								            EXPECT_EQ(sample.time, time);
								            EXPECT_EQ(
								                sample.data.size(),
								                projection_shapes.at(p).get().size());
								            for (size_t i = 0; i < sample.data.size(); ++i) {
									            EXPECT_EQ(
									                static_cast<int>(sample.data.at(i).at(0)),
									                expectations.at(j))
									                << name;
								            }
								            time++;
							            }
						            }
					            }
				            },
				            obsv.type);
			        },
			        [&](PlasticityRule::TimedRecordingConfig::ObservablePerNeuron const& obsv) {
				        EXPECT_TRUE(timed_recording_data.data_per_neuron.contains(name));
				        auto const& data_entry_variant =
				            timed_recording_data.data_per_neuron.at(name);
				        std::visit(
				            [&](auto type) {
					            EXPECT_EQ(data_entry_variant.size(), population_shapes.size());
					            for (size_t p = 0; p < population_shapes.size(); ++p) {
						            auto const& data_entry = std::get<std::vector<
						                grenade::vx::common::TimedDataSequence<std::vector<std::map<
						                    CompartmentOnLogicalNeuron,
						                    std::vector<typename decltype(type)::ElementType>>>>>>(
						                data_entry_variant.at(p));
						            EXPECT_EQ(data_entry.size(), input_data.batch_size());
						            for (size_t i = 0; i < data_entry.size(); ++i) {
							            auto const& samples = data_entry.at(i);
							            EXPECT_EQ(
							                samples.size(),
							                plasticity_rule_dynamics.timer.num_periods);
							            for (size_t time = 0; auto const& sample : samples) {
								            EXPECT_EQ(sample.time, time);
								            EXPECT_EQ(
								                sample.data.size(),
								                population_shapes.at(p).get().size());
								            for (size_t i = 0; i < sample.data.size(); ++i) {
									            EXPECT_EQ(
									                static_cast<int>(
									                    sample.data.at(i)
									                        .at(CompartmentOnLogicalNeuron())
									                        .at(0)),
									                expectations.at(j))
									                << name;
								            }
								            time++;
							            }
						            }
					            }
				            },
				            obsv.type);
			        },
			        [](auto const&) {
				        throw std::logic_error("Observable type not implemented.");
			        }},
			    observable);
			j++;
		}
	}
}

TEST(PlasticityRule, ExecutorInitialState)
{
	InputData input_data;
	input_data.time_domain_runtimes.set(
	    TimeDomainOnTopology(),
	    ClockCycleTimeDomainRuntimes(
	        {std::optional<grenade::vx::common::Time>(
	            grenade::vx::common::Time::fpga_clock_cycles_per_us * 1000)},
	        grenade::vx::common::Time()));

	// Construct connection to HW
	grenade::vx::execution::JITGraphExecutor executor;

	size_t const weight_63 = 63;
	size_t const weight_32 = 32;

	auto const execute = [&](bool const set_weight) {
		auto topology = std::make_shared<Topology>();

		auto const chip = get_chip_config_bypass_excitatory();
		Population population{
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
		UncalibratedNeuron::ParameterSpace::Parameterization population_input_data;
		population_input_data.configs.resize(
		    1, {{grenade::common::CompartmentOnNeuron(),
		         {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
		population_input_data.base_configs.emplace_back(std::vector<size_t>{0}, chip);
		auto const population_descriptor = topology->add_vertex(population);

		Projection projection(
		    UncalibratedSynapse{},
		    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
		    SequenceConnector{
		        CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}),
		        ListMultiIndexSequence({MultiIndex({0, 0})})},
		    TimeDomainOnTopology());
		UncalibratedSynapse::ParameterSpace::Parameterization projection_input_data(
		    {UncalibratedSynapse::Weight(63)});

		auto const projection_descriptor = topology->add_vertex(projection);

		topology->add_edge(
		    population_descriptor, projection_descriptor,
		    Edge(
		        CuboidMultiIndexSequence(
		            {1, 1}, MultiIndex({0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
		        CuboidMultiIndexSequence({1}), 0, 0));

		topology->add_edge(
		    projection_descriptor, population_descriptor,
		    Edge(
		        CuboidMultiIndexSequence({1}),
		        CuboidMultiIndexSequence(
		            {1, 1, 1}, MultiIndex({0, 0, 0}),
		            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
		             ReceptorOnCompartmentDimensionUnit()}),
		        0, 0));

		std::vector<CuboidMultiIndexSequence> projection_shapes{CuboidMultiIndexSequence({1})};

		PlasticityRule plasticity_rule(
		    PlasticityRule::TimedRecordingConfig{
		        {{"w",
		          PlasticityRule::TimedRecordingConfig::ObservablePerSynapse{
		              PlasticityRule::TimedRecordingConfig::ObservablePerSynapse::Type::int8,
		              PlasticityRule::TimedRecordingConfig::ObservablePerSynapse::LayoutPerRow::
		                  packed_active_columns}}},
		        true},
		    PlasticityRule::ID(), {}, {projection_shapes.at(0)}, TimeDomainOnTopology());

		PlasticityRule::Dynamics plasticity_rule_dynamics(
		    PlasticityRule::Dynamics::Timer{
		        PlasticityRule::Dynamics::Timer::Value(0),
		        PlasticityRule::Dynamics::Timer::Value(10000), 1},
		    1);

		std::stringstream kernel;
		kernel << "#include \"grenade/vx/ppu/synapse_array_view_handle.h\"\n";
		kernel << "#include \"grenade/vx/ppu/neuron_view_handle.h\"\n";
		kernel << "#include \"libnux/vx/location.h\"\n";
		kernel << "using namespace grenade::vx::ppu;\n";
		kernel << "using namespace libnux::vx;\n";
		kernel << "extern volatile PPUOnDLS ppu;\n";
		kernel << "void PLASTICITY_RULE_KERNEL(std::array<SynapseArrayViewHandle, 1>& synapses, "
		          "std::array<NeuronViewHandle, 0>&, "
		          "Recording& recording)\n";
		kernel << "{\n";
		kernel << "  if (synapses[0].hemisphere != ppu) {\n";
		kernel << "    return;\n";
		kernel << "  }\n";
		kernel << "  auto w = synapses[0].get_weights(0);\n";
		if (set_weight) {
			kernel << "  w = " << static_cast<int>(weight_32) << ";\n";
		}
		kernel << "  for (size_t i = 0; i < std::get<0>(recording.w).size(); ++i) {\n";
		kernel << "    size_t active_column = 0;\n";
		kernel << "    for (size_t column: synapses[0].columns) {\n";
		kernel << "        std::get<0>(recording.w)[i][active_column] = w[column];\n";
		kernel << "        active_column++;\n";
		kernel << "    }\n";
		kernel << "  synapses[0].set_weights(w, 0);\n";
		kernel << "  }\n";
		kernel << "}\n";

		PlasticityRule::Parameterization plasticity_rule_parameterization(kernel.str());

		auto const plasticity_rule_descriptor = topology->add_vertex(plasticity_rule);

		topology->add_edge(
		    projection_descriptor, plasticity_rule_descriptor,
		    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}), 1, 0));

		// build network graph
		FixtureCalibration const calibration;
		GreedyMapper mapper;
		auto const mapped_topology =
		    std::make_shared<LinkedTopology>(mapper(topology, calibration, executor));

		input_data.ports.set({population_descriptor, 1}, population_input_data);
		input_data.ports.set({projection_descriptor, 1}, projection_input_data);
		input_data.ports.set({plasticity_rule_descriptor, 1}, plasticity_rule_parameterization);
		input_data.ports.set({plasticity_rule_descriptor, 2}, plasticity_rule_dynamics);

		auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

		// run graph with given inputs and return results
		auto const mapped_results =
		    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

		auto const results = mapped_topology->map_root_output_data(mapped_results);

		auto const timed_recording_data = std::get<PlasticityRule::Results::TimedData>(
		    dynamic_cast<PlasticityRule::Results const&>(
		        results.ports.get({plasticity_rule_descriptor, 0}))
		        .data);

		auto const result = std::get<
		    std::vector<grenade::vx::common::TimedDataSequence<std::vector<std::vector<int8_t>>>>>(
		    timed_recording_data.data_per_synapse.at("w").at(0));
		EXPECT_EQ(result.size(), input_data.batch_size());
		EXPECT_EQ(result.at(0).size(), plasticity_rule_dynamics.timer.num_periods);
		return static_cast<int>(result.at(0).at(0).data.at(0).at(0));
	};

	auto const with_weight_32 = execute(true);
	auto const with_weight_63 = execute(false);
	EXPECT_EQ(with_weight_32, weight_32);
	EXPECT_EQ(with_weight_63, weight_63);
}

TEST(PlasticityRule, SynapseRowViewHandleRange)
{
	InputData input_data;
	input_data.time_domain_runtimes.set(
	    TimeDomainOnTopology(),
	    ClockCycleTimeDomainRuntimes(
	        {std::optional<grenade::vx::common::Time>(
	            grenade::vx::common::Time::fpga_clock_cycles_per_us * 1000)},
	        grenade::vx::common::Time()));

	// Construct connection to HW
	grenade::vx::execution::JITGraphExecutor executor;

	auto topology = std::make_shared<Topology>();

	auto const chip = get_chip_config_bypass_excitatory();
	Population population{
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
	UncalibratedNeuron::ParameterSpace::Parameterization population_input_data;
	population_input_data.configs.resize(
	    1, {{grenade::common::CompartmentOnNeuron(),
	         {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
	population_input_data.base_configs.emplace_back(std::vector<size_t>{0}, chip);

	auto const population_descriptor = topology->add_vertex(population);

	Projection projection(
	    UncalibratedSynapse{},
	    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
	    SequenceConnector{
	        CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}),
	        ListMultiIndexSequence({MultiIndex({0, 0})})},
	    TimeDomainOnTopology());
	UncalibratedSynapse::ParameterSpace::Parameterization projection_input_data(
	    {UncalibratedSynapse::Weight(1)});

	auto const projection_descriptor = topology->add_vertex(projection);

	topology->add_edge(
	    population_descriptor, projection_descriptor,
	    Edge(
	        CuboidMultiIndexSequence(
	            {1, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({1}), 0, 0));

	topology->add_edge(
	    projection_descriptor, population_descriptor,
	    Edge(
	        CuboidMultiIndexSequence({1}),
	        CuboidMultiIndexSequence(
	            {1, 1, 1}, MultiIndex({0, 0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
	             ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	std::vector<CuboidMultiIndexSequence> projection_shapes{CuboidMultiIndexSequence({1})};

	PlasticityRule::Dynamics plasticity_rule_dynamics(
	    PlasticityRule::Dynamics::Timer{
	        PlasticityRule::Dynamics::Timer::Value(0),
	        PlasticityRule::Dynamics::Timer::Value(10000), 1},
	    1);

	PlasticityRule plasticity_rule(
	    std::nullopt, PlasticityRule::ID(), {}, {projection_shapes.at(0)}, TimeDomainOnTopology());

	std::stringstream kernel;
	kernel << "#include \"grenade/vx/ppu/synapse_array_view_handle.h\"\n";
	kernel << "#include \"grenade/vx/ppu/synapse_row_view_handle_range.h\"\n";
	kernel << "#include \"grenade/vx/ppu/neuron_view_handle.h\"\n";
	kernel << "#include \"libnux/vx/location.h\"\n";
	kernel << "using namespace grenade::vx::ppu;\n";
	kernel << "using namespace libnux::vx;\n";
	kernel << "extern volatile PPUOnDLS ppu;\n";
	kernel << "void PLASTICITY_RULE_KERNEL(std::array<SynapseArrayViewHandle, 1>& synapses, "
	          "std::array<NeuronViewHandle, 0>&)\n";
	kernel << "{\n";
	kernel << "  if (synapses[0].hemisphere != ppu) {\n";
	kernel << "    return;\n";
	kernel << "  }\n";
	kernel << "  for (auto const synapse_row: make_synapse_row_view_handle_range(synapses)) {\n";
	kernel << "  }\n";
	kernel << "}\n";

	PlasticityRule::Parameterization plasticity_rule_parameterization(kernel.str());

	auto const plasticity_rule_descriptor = topology->add_vertex(plasticity_rule);

	topology->add_edge(
	    projection_descriptor, plasticity_rule_descriptor,
	    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}), 1, 0));

	// build network graph
	FixtureCalibration const calibration;
	GreedyMapper mapper;
	auto const mapped_topology =
	    std::make_shared<LinkedTopology>(mapper(topology, calibration, executor));

	input_data.ports.set({population_descriptor, 1}, population_input_data);
	input_data.ports.set({projection_descriptor, 1}, projection_input_data);
	input_data.ports.set({plasticity_rule_descriptor, 1}, plasticity_rule_parameterization);
	input_data.ports.set({plasticity_rule_descriptor, 2}, plasticity_rule_dynamics);

	auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

	// run graph with given inputs and return results
	auto const mapped_results =
	    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

	EXPECT_NO_THROW(mapped_topology->map_root_output_data(mapped_results));
}

TEST(PlasticityRule, SynapseRowViewHandleSignedRange)
{
	InputData input_data;
	input_data.time_domain_runtimes.set(
	    TimeDomainOnTopology(),
	    ClockCycleTimeDomainRuntimes(
	        {std::optional<grenade::vx::common::Time>(
	            grenade::vx::common::Time::fpga_clock_cycles_per_us * 1000)},
	        grenade::vx::common::Time()));

	// Construct connection to HW
	grenade::vx::execution::JITGraphExecutor executor;

	auto topology = std::make_shared<Topology>();

	auto const chip = get_chip_config_bypass_excitatory();
	Population population{
	    UncalibratedNeuron{
	        UncalibratedNeuron::Compartments{
	            {grenade::common::CompartmentOnNeuron(),
	             UncalibratedNeuron::Compartment{
	                 UncalibratedNeuron::Compartment::SpikeMaster(0),
	                 {{{grenade::common::ReceptorOnCompartment(0),
	                    grenade::vx::network::Receptor::Type::excitatory},
	                   {grenade::common::ReceptorOnCompartment(1),
	                    grenade::vx::network::Receptor::Type::inhibitory}}}}}},
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
	    grenade::common::CuboidMultiIndexSequence(
	        {1}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    UncalibratedNeuron::ParameterSpace(1, {{CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};
	UncalibratedNeuron::ParameterSpace::Parameterization population_input_data;
	population_input_data.configs.resize(
	    1, {{grenade::common::CompartmentOnNeuron(),
	         {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
	population_input_data.base_configs.emplace_back(std::vector<size_t>{0}, chip);

	auto const population_descriptor = topology->add_vertex(population);

	Projection projection(
	    UncalibratedSynapse{},
	    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
	    SequenceConnector{
	        CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}),
	        ListMultiIndexSequence({MultiIndex({0, 0})})},
	    TimeDomainOnTopology());
	UncalibratedSynapse::ParameterSpace::Parameterization projection_input_data(
	    {UncalibratedSynapse::Weight(1)});

	auto const projection_exc_descriptor = topology->add_vertex(projection);
	auto const projection_inh_descriptor = topology->add_vertex(projection);

	topology->add_edge(
	    population_descriptor, projection_exc_descriptor,
	    Edge(
	        CuboidMultiIndexSequence(
	            {1, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({1}), 0, 0));

	topology->add_edge(
	    projection_exc_descriptor, population_descriptor,
	    Edge(
	        CuboidMultiIndexSequence({1}),
	        CuboidMultiIndexSequence(
	            {1, 1, 1}, MultiIndex({0, 0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
	             ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	topology->add_edge(
	    population_descriptor, projection_inh_descriptor,
	    Edge(
	        CuboidMultiIndexSequence(
	            {1, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({1}), 0, 0));

	topology->add_edge(
	    projection_inh_descriptor, population_descriptor,
	    Edge(
	        CuboidMultiIndexSequence({1}),
	        CuboidMultiIndexSequence(
	            {1, 1, 1}, MultiIndex({0, 0, 1}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
	             ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	std::vector<CuboidMultiIndexSequence> projection_shapes{
	    CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1})};

	PlasticityRule::Dynamics plasticity_rule_dynamics(
	    PlasticityRule::Dynamics::Timer{
	        PlasticityRule::Dynamics::Timer::Value(0),
	        PlasticityRule::Dynamics::Timer::Value(10000), 1},
	    1);

	PlasticityRule plasticity_rule(
	    std::nullopt, PlasticityRule::ID(), {}, {projection_shapes.at(0), projection_shapes.at(1)},
	    TimeDomainOnTopology());

	std::stringstream kernel;
	kernel << "#include \"grenade/vx/ppu/synapse_array_view_handle.h\"\n";
	kernel << "#include \"grenade/vx/ppu/synapse_row_view_handle_range.h\"\n";
	kernel << "#include \"grenade/vx/ppu/neuron_view_handle.h\"\n";
	kernel << "#include \"libnux/vx/location.h\"\n";
	kernel << "using namespace grenade::vx::ppu;\n";
	kernel << "using namespace libnux::vx;\n";
	kernel << "extern volatile PPUOnDLS ppu;\n";
	kernel << "void PLASTICITY_RULE_KERNEL(std::array<SynapseArrayViewHandle, 2>& synapses, "
	          "std::array<NeuronViewHandle, 0>&)\n";
	kernel << "{\n";
	kernel << "  if (synapses[0].hemisphere != ppu) {\n";
	kernel << "    return;\n";
	kernel << "  }\n";
	kernel << "  for (auto const synapse_row: make_synapse_row_view_handle_range(synapses)) "
	          "{\n";
	kernel << "  }\n";
	kernel << "}\n";

	PlasticityRule::Parameterization plasticity_rule_parameterization(kernel.str());

	auto const plasticity_rule_descriptor = topology->add_vertex(plasticity_rule);

	topology->add_edge(
	    projection_exc_descriptor, plasticity_rule_descriptor,
	    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}), 1, 0));

	topology->add_edge(
	    projection_inh_descriptor, plasticity_rule_descriptor,
	    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}), 1, 1));

	// build network graph
	FixtureCalibration const calibration;
	GreedyMapper mapper;
	auto const mapped_topology =
	    std::make_shared<LinkedTopology>(mapper(topology, calibration, executor));

	input_data.ports.set({population_descriptor, 1}, population_input_data);
	input_data.ports.set({projection_exc_descriptor, 1}, projection_input_data);
	input_data.ports.set({projection_inh_descriptor, 1}, projection_input_data);
	input_data.ports.set({plasticity_rule_descriptor, 2}, plasticity_rule_parameterization);
	input_data.ports.set({plasticity_rule_descriptor, 3}, plasticity_rule_dynamics);

	auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

	// run graph with given inputs and return results
	auto const mapped_results =
	    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data);

	EXPECT_NO_THROW(mapped_topology->map_root_output_data(mapped_results));
}

TEST(PlasticityRule, WriteReadPPUSymbol)
{
	InputData input_data;
	input_data.time_domain_runtimes.set(
	    TimeDomainOnTopology(),
	    ClockCycleTimeDomainRuntimes(
	        {std::optional<grenade::vx::common::Time>(
	            grenade::vx::common::Time::fpga_clock_cycles_per_us * 1000)},
	        grenade::vx::common::Time()));

	// Construct connection to HW
	grenade::vx::execution::JITGraphExecutor executor;

	auto topology = std::make_shared<Topology>();

	auto const chip = get_chip_config_bypass_excitatory();
	Population population{
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
	UncalibratedNeuron::ParameterSpace::Parameterization population_input_data;
	population_input_data.configs.resize(
	    1, {{grenade::common::CompartmentOnNeuron(),
	         {chip.neuron_block.atomic_neurons.at(AtomicNeuronOnDLS())}}});
	population_input_data.base_configs.emplace_back(std::vector<size_t>{0}, chip);

	auto const population_descriptor = topology->add_vertex(population);

	Projection projection(
	    UncalibratedSynapse{},
	    UncalibratedSynapse::ParameterSpace{{UncalibratedSynapse::Weight(63)}},
	    SequenceConnector{
	        CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}),
	        ListMultiIndexSequence({MultiIndex({0, 0})})},
	    TimeDomainOnTopology());
	UncalibratedSynapse::ParameterSpace::Parameterization projection_input_data(
	    {UncalibratedSynapse::Weight(1)});

	auto const projection_descriptor = topology->add_vertex(projection);

	topology->add_edge(
	    population_descriptor, projection_descriptor,
	    Edge(
	        CuboidMultiIndexSequence(
	            {1, 1}, MultiIndex({0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit()}),
	        CuboidMultiIndexSequence({1}), 0, 0));

	topology->add_edge(
	    projection_descriptor, population_descriptor,
	    Edge(
	        CuboidMultiIndexSequence({1}),
	        CuboidMultiIndexSequence(
	            {1, 1, 1}, MultiIndex({0, 0, 0}),
	            {CellOnPopulationDimensionUnit(), CompartmentOnNeuronDimensionUnit(),
	             ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	std::vector<CuboidMultiIndexSequence> projection_shapes{CuboidMultiIndexSequence({1})};

	PlasticityRule::Dynamics plasticity_rule_dynamics(
	    PlasticityRule::Dynamics::Timer{
	        PlasticityRule::Dynamics::Timer::Value(0),
	        PlasticityRule::Dynamics::Timer::Value(10000), 1},
	    1);

	PlasticityRule plasticity_rule(
	    std::nullopt, PlasticityRule::ID(), {}, {projection_shapes.at(0)}, TimeDomainOnTopology());

	std::stringstream kernel;
	kernel << "#include \"grenade/vx/ppu/synapse_array_view_handle.h\"\n";
	kernel << "#include \"grenade/vx/ppu/neuron_view_handle.h\"\n";
	kernel << "#include <array>\n";
	kernel << "using namespace grenade::vx::ppu;\n";
	kernel << "volatile uint32_t test;\n";
	kernel << "void PLASTICITY_RULE_KERNEL(std::array<SynapseArrayViewHandle, 1>& synapses, "
	          "std::array<NeuronViewHandle, 0>&)\n";
	kernel << "{\n";
	kernel << "  static_cast<void>(test);\n";
	kernel << "}\n";

	PlasticityRule::Parameterization plasticity_rule_parameterization(kernel.str());

	auto const plasticity_rule_descriptor = topology->add_vertex(plasticity_rule);

	topology->add_edge(
	    projection_descriptor, plasticity_rule_descriptor,
	    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1}), 1, 0));

	// build network graph
	FixtureCalibration const calibration;
	GreedyMapper mapper;
	auto const mapped_topology =
	    std::make_shared<LinkedTopology>(mapper(topology, calibration, executor));

	input_data.ports.set({population_descriptor, 1}, population_input_data);
	input_data.ports.set({projection_descriptor, 1}, projection_input_data);
	input_data.ports.set({plasticity_rule_descriptor, 1}, plasticity_rule_parameterization);
	input_data.ports.set({plasticity_rule_descriptor, 2}, plasticity_rule_dynamics);

	auto const mapped_input_data = mapped_topology->map_root_input_data(input_data);

	grenade::vx::execution::JITGraphExecutor::Hooks hooks;
	haldls::vx::v3::PPUMemoryBlock expectation(halco::hicann_dls::vx::v3::PPUMemoryBlockSize(1));
	expectation.at(0) =
	    haldls::vx::v3::PPUMemoryWord(haldls::vx::v3::PPUMemoryWord::Value(0x12345678));
	hooks[grenade::common::ExecutionInstanceOnExecutor()] =
	    std::make_shared<grenade::vx::execution::ExecutionInstanceHooks>();
	hooks[grenade::common::ExecutionInstanceOnExecutor()]
	    ->chips[grenade::vx::common::ChipOnConnection()]
	    .write_ppu_symbols["test"] =
	    std::map<halco::hicann_dls::vx::v3::HemisphereOnDLS, haldls::vx::v3::PPUMemoryBlock>{
	        {halco::hicann_dls::vx::v3::HemisphereOnDLS::top, expectation},
	        {halco::hicann_dls::vx::v3::HemisphereOnDLS::bottom, expectation},
	    };
	hooks[grenade::common::ExecutionInstanceOnExecutor()]
	    ->chips[grenade::vx::common::ChipOnConnection()]
	    .read_ppu_symbols.insert("test");

	auto const expectation_symbols = hooks.at(grenade::common::ExecutionInstanceOnExecutor())
	                                     ->chips[grenade::vx::common::ChipOnConnection()]
	                                     .write_ppu_symbols;

	// run graph with given inputs and return results
	auto const mapped_results =
	    grenade::vx::execution::run(executor, mapped_topology, mapped_input_data, std::move(hooks));

	auto const results = mapped_topology->map_root_output_data(mapped_results);

	EXPECT_EQ(
	    dynamic_cast<ExecutionInstanceGlobal const&>(
	        results.execution_instances.get(ExecutionInstanceOnExecutor()))
	        .read_ppu_symbols.at(0)
	        .at(grenade::vx::common::ChipOnConnection()),
	    expectation_symbols);
}
