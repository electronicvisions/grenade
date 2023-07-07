#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/extract_output.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/plasticity_rule.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/routing/portfolio_router.h"
#include "grenade/vx/signal_flow/execution_instance.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/variant.h"
#include <gtest/gtest.h>
#include <log4cxx/logger.h>

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace stadls::vx::v3;
using namespace lola::vx::v3;
using namespace haldls::vx::v3;
using namespace grenade::vx::network;

TEST(PlasticityRule, RawRecording)
{
	grenade::vx::signal_flow::ExecutionInstance instance;

	grenade::vx::execution::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[instance] = lola::vx::v3::Chip();

	// build network
	NetworkBuilder network_builder;

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
	Population population{std::move(neurons)};
	auto const population_descriptor = network_builder.add(population);

	Projection::Connections projection_connections;
	for (size_t i = 0; i < population.neurons.size(); ++i) {
		projection_connections.push_back(
		    {{i, CompartmentOnLogicalNeuron()},
		     {i, CompartmentOnLogicalNeuron()},
		     Projection::Connection::Weight(63)});
	}
	Projection projection{
	    Receptor(Receptor::ID(), Receptor::Type::excitatory), std::move(projection_connections),
	    population_descriptor, population_descriptor};
	auto const projection_descriptor = network_builder.add(projection);

	PlasticityRule plasticity_rule;
	plasticity_rule.timer.start = PlasticityRule::Timer::Value(1000);
	plasticity_rule.timer.period = PlasticityRule::Timer::Value(10000);
	plasticity_rule.timer.num_periods = 1;
	plasticity_rule.projections.push_back(projection_descriptor);
	plasticity_rule.recording = PlasticityRule::RawRecording{8};

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
	plasticity_rule.kernel = kernel.str();

	auto const plasticity_rule_descriptor = network_builder.add(plasticity_rule);

	auto const network = network_builder.done();
	auto const routing_result = routing::PortfolioRouter()(network);
	auto const network_graph = build_network_graph(network, routing_result);

	grenade::vx::signal_flow::IODataMap inputs;
	inputs.runtime.push_back(
	    {{instance,
	      grenade::vx::common::Time(grenade::vx::common::Time::fpga_clock_cycles_per_us * 1000)}});
	inputs.runtime.push_back(
	    {{instance,
	      grenade::vx::common::Time(grenade::vx::common::Time::fpga_clock_cycles_per_us * 1000)}});

	// Construct connection to HW
	grenade::vx::execution::JITGraphExecutor executor;

	// run graph with given inputs and return results
	auto const result_map =
	    grenade::vx::execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

	auto const result = std::get<PlasticityRule::RawRecordingData>(
	                        extract_plasticity_rule_recording_data(
	                            result_map, network_graph, plasticity_rule_descriptor))
	                        .data;

	EXPECT_EQ(result.size(), inputs.batch_size());
	for (size_t i = 0; i < result.size(); ++i) {
		auto const& samples = result.at(i);
		EXPECT_EQ(samples.size(), 8 * 2);
		for (size_t j = 0; auto const& sample : samples) {
			EXPECT_EQ(static_cast<int>(sample), j % 8);
			j++;
		}
	}
}

TEST(PlasticityRule, TimedRecording)
{
	grenade::vx::signal_flow::ExecutionInstance instance;

	grenade::vx::execution::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[instance] = lola::vx::v3::Chip();

	grenade::vx::signal_flow::IODataMap inputs;
	inputs.runtime.push_back(
	    {{instance,
	      grenade::vx::common::Time(grenade::vx::common::Time::fpga_clock_cycles_per_us * 10000)}});
	inputs.runtime.push_back(
	    {{instance,
	      grenade::vx::common::Time(grenade::vx::common::Time::fpga_clock_cycles_per_us * 10000)}});

	grenade::vx::execution::backend::Connection connection;
	std::map<DLSGlobal, grenade::vx::execution::backend::Connection> connections;
	connections.emplace(DLSGlobal(), std::move(connection));
	grenade::vx::execution::JITGraphExecutor executor(std::move(connections));

	std::mt19937 rng(std::random_device{}());
	constexpr size_t num = 10;
	for (size_t n = 0; n < num; ++n) {
		Population::Neurons all_neurons;
		for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
			all_neurons.push_back(Population::Neuron(
			    LogicalNeuronOnDLS(
			        LogicalNeuronCompartments(
			            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
			        neuron),
			    Population::Neuron::Compartments{
			        {CompartmentOnLogicalNeuron(),
			         Population::Neuron::Compartment{
			             Population::Neuron::Compartment::SpikeMaster(0, false),
			             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}));
		}

		// build network
		NetworkBuilder network_builder;

		// FIXME: Fix is required in order in routing
		std::uniform_int_distribution num_projection_distribution(1, 3);
		size_t const num_projections = num_projection_distribution(rng);
		std::vector<ProjectionDescriptor> projection_descriptors;
		std::vector<PlasticityRule::PopulationHandle> population_descriptors;
		for (size_t p = 0; p < num_projections; ++p) {
			std::uniform_int_distribution neuron_distribution(1, 3);

			Population::Neurons neurons_s;
			std::sample(
			    all_neurons.begin(), all_neurons.end(), std::back_inserter(neurons_s),
			    neuron_distribution(rng), rng);
			for (auto const& neuron : neurons_s) {
				all_neurons.erase(std::find(all_neurons.begin(), all_neurons.end(), neuron));
			}
			// sort neurons in population because otherwise dense requirement of projection is not
			// necessarily fulfilled
			std::stable_sort(neurons_s.begin(), neurons_s.end(), [](auto const& a, auto const& b) {
				return a.coordinate.get_atomic_neurons()
				           .at(0)
				           .toNeuronColumnOnDLS()
				           .toNeuronEventOutputOnDLS()
				           .toNeuronEventOutputOnNeuronBackendBlock() <
				       b.coordinate.get_atomic_neurons()
				           .at(0)
				           .toNeuronColumnOnDLS()
				           .toNeuronEventOutputOnDLS()
				           .toNeuronEventOutputOnNeuronBackendBlock();
			});
			Population population_s{std::move(neurons_s)};
			auto const population_descriptor_s = network_builder.add(population_s);

			Population::Neurons neurons_t;
			std::sample(
			    all_neurons.begin(), all_neurons.end(), std::back_inserter(neurons_t),
			    neuron_distribution(rng), rng);
			for (auto const& neuron : neurons_t) {
				all_neurons.erase(std::find(all_neurons.begin(), all_neurons.end(), neuron));
			}
			// sort neurons in population because otherwise dense requirement of projection is not
			// necessarily fulfilled
			std::stable_sort(neurons_t.begin(), neurons_t.end(), [](auto const& a, auto const& b) {
				return a.coordinate.get_atomic_neurons()
				           .at(0)
				           .toNeuronColumnOnDLS()
				           .toNeuronEventOutputOnDLS()
				           .toNeuronEventOutputOnNeuronBackendBlock() <
				       b.coordinate.get_atomic_neurons()
				           .at(0)
				           .toNeuronColumnOnDLS()
				           .toNeuronEventOutputOnDLS()
				           .toNeuronEventOutputOnNeuronBackendBlock();
			});
			Population population_t{std::move(neurons_t)};
			auto const population_descriptor_t = network_builder.add(population_t);

			Projection::Connections projection_connections;
			for (size_t i = 0; i < population_s.neurons.size(); ++i) {
				for (size_t j = 0; j < population_t.neurons.size(); ++j) {
					projection_connections.push_back(
					    {{i, CompartmentOnLogicalNeuron()},
					     {j, CompartmentOnLogicalNeuron()},
					     Projection::Connection::Weight(63)});
				}
			}
			Projection projection{
			    Receptor(Receptor::ID(), Receptor::Type::excitatory),
			    std::move(projection_connections), population_descriptor_s,
			    population_descriptor_t};
			projection_descriptors.push_back(network_builder.add(projection));
			PlasticityRule::PopulationHandle population_handle_t;
			population_handle_t.descriptor = population_descriptor_t;
			population_handle_t.neuron_readout_sources.resize(
			    population_t.neurons.size(), {{CompartmentOnLogicalNeuron(), {std::nullopt}}});
			population_descriptors.push_back(population_handle_t);
		}

		PlasticityRule plasticity_rule;
		plasticity_rule.timer.start = PlasticityRule::Timer::Value(0);
		plasticity_rule.timer.period = PlasticityRule::Timer::Value(
		    grenade::vx::common::Time::fpga_clock_cycles_per_us * 3000);

		std::uniform_int_distribution num_periods_distribution(1, 3);
		plasticity_rule.timer.num_periods = num_periods_distribution(rng);

		plasticity_rule.projections = projection_descriptors;
		plasticity_rule.populations = population_descriptors;

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
		PlasticityRule::TimedRecording recording;
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
					auto obsv =
					    grenade::vx::network::PlasticityRule::TimedRecording::ObservableArray{
					        PlasticityRule::TimedRecording::ObservableArray::Type::int8,
					        std::uniform_int_distribution<size_t>(1, 15)(rng)};
					// select data type
					switch (std::uniform_int_distribution{0, 3}(rng)) {
						case 0: { // int8
							obsv.type = PlasticityRule::TimedRecording::ObservableArray::Type::int8;
							expectations.push_back(std::uniform_int_distribution<int8_t>()(rng));
							break;
						}
						case 1: { // uint8
							obsv.type =
							    PlasticityRule::TimedRecording::ObservableArray::Type::uint8;
							expectations.push_back(std::uniform_int_distribution<uint8_t>()(rng));
							break;
						}
						case 2: { // int16
							obsv.type =
							    PlasticityRule::TimedRecording::ObservableArray::Type::int16;
							expectations.push_back(std::uniform_int_distribution<int16_t>()(rng));
							break;
						}
						case 3: { // uint16
							obsv.type =
							    PlasticityRule::TimedRecording::ObservableArray::Type::uint16;
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
					auto obsv = PlasticityRule::TimedRecording::ObservablePerSynapse{
					    PlasticityRule::TimedRecording::ObservablePerSynapse::Type::int8,
					    std::bernoulli_distribution{}(rng)
					        ? PlasticityRule::TimedRecording::ObservablePerSynapse::LayoutPerRow::
					              complete_rows
					        : PlasticityRule::TimedRecording::ObservablePerSynapse::LayoutPerRow::
					              packed_active_columns};
					// select data type
					switch (std::uniform_int_distribution{0, 3}(rng)) {
						case 0: { // int8
							obsv.type =
							    PlasticityRule::TimedRecording::ObservablePerSynapse::Type::int8;
							expectations.push_back(std::uniform_int_distribution<int8_t>()(rng));
							break;
						}
						case 1: { // uint8
							obsv.type =
							    PlasticityRule::TimedRecording::ObservablePerSynapse::Type::uint8;
							expectations.push_back(std::uniform_int_distribution<uint8_t>()(rng));
							break;
						}
						case 2: { // int16
							obsv.type =
							    PlasticityRule::TimedRecording::ObservablePerSynapse::Type::int16;
							expectations.push_back(std::uniform_int_distribution<int16_t>()(rng));
							break;
						}
						case 3: { // uint16
							obsv.type =
							    PlasticityRule::TimedRecording::ObservablePerSynapse::Type::uint16;
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
						case PlasticityRule::TimedRecording::ObservablePerSynapse::LayoutPerRow::
						    complete_rows: {
							kernel << "for (auto& row: rec) { row = "
							       << expectations.at(expectations.size() - 1) << "; } },\n";
							break;
						}
						case PlasticityRule::TimedRecording::ObservablePerSynapse::LayoutPerRow::
						    packed_active_columns: {
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
					auto obsv = PlasticityRule::TimedRecording::ObservablePerNeuron{
					    PlasticityRule::TimedRecording::ObservablePerNeuron::Type::int8,
					    std::bernoulli_distribution{}(rng)
					        ? PlasticityRule::TimedRecording::ObservablePerNeuron::Layout::
					              complete_row
					        : PlasticityRule::TimedRecording::ObservablePerNeuron::Layout::
					              packed_active_columns};
					// select data type
					switch (std::uniform_int_distribution{0, 3}(rng)) {
						case 0: { // int8
							obsv.type =
							    PlasticityRule::TimedRecording::ObservablePerNeuron::Type::int8;
							expectations.push_back(std::uniform_int_distribution<int8_t>()(rng));
							break;
						}
						case 1: { // uint8
							obsv.type =
							    PlasticityRule::TimedRecording::ObservablePerNeuron::Type::uint8;
							expectations.push_back(std::uniform_int_distribution<uint8_t>()(rng));
							break;
						}
						case 2: { // int16
							obsv.type =
							    PlasticityRule::TimedRecording::ObservablePerNeuron::Type::int16;
							expectations.push_back(std::uniform_int_distribution<int16_t>()(rng));
							break;
						}
						case 3: { // uint16
							obsv.type =
							    PlasticityRule::TimedRecording::ObservablePerNeuron::Type::uint16;
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
						case PlasticityRule::TimedRecording::ObservablePerNeuron::Layout::
						    complete_row: {
							kernel << "rec = " << expectations.at(expectations.size() - 1) << ";\n";
							break;
						}
						case PlasticityRule::TimedRecording::ObservablePerNeuron::Layout::
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

		plasticity_rule.recording = recording;

		kernel << "\trecording.time = period * 2 /* f_ppu = 2 * f_fpga */;\n";
		kernel << "\tperiod++;\n";
		kernel << "\tperiod %= " << plasticity_rule.timer.num_periods << ";\n";
		kernel << "}";
		plasticity_rule.kernel = kernel.str();

		auto const plasticity_rule_descriptor = network_builder.add(plasticity_rule);

		auto const network = network_builder.done();
		auto const routing_result = routing::PortfolioRouter()(network);
		auto const network_graph = build_network_graph(network, routing_result);

		// run graph with given inputs and return results
		EXPECT_NO_THROW((grenade::vx::execution::run(
		    executor, network_graph.get_graph(), inputs, chip_configs)));

		auto const result_map =
		    grenade::vx::execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

		auto const recording_data = extract_plasticity_rule_recording_data(
		    result_map, network_graph, plasticity_rule_descriptor);
		auto const& timed_recording_data =
		    std::get<PlasticityRule::TimedRecordingData>(recording_data);

		for (size_t j = 0; auto const& [name, observable] : recording.observables) {
			std::visit(
			    hate::overloaded{
			        [&](PlasticityRule::TimedRecording::ObservableArray const& obsv) {
				        EXPECT_TRUE(timed_recording_data.data_array.contains(name));
				        auto const& data_entry_variant = timed_recording_data.data_array.at(name);
				        std::visit(
				            [&](auto type) {
					            auto const& data_entry =
					                std::get<std::vector<grenade::vx::common::TimedDataSequence<
					                    std::vector<typename decltype(type)::ElementType>>>>(
					                    data_entry_variant);
					            EXPECT_EQ(data_entry.size(), inputs.batch_size());
					            for (size_t i = 0; i < data_entry.size(); ++i) {
						            auto const& samples = data_entry.at(i);
						            EXPECT_EQ(samples.size(), plasticity_rule.timer.num_periods);
						            for (size_t time = 0; auto const& sample : samples) {
							            EXPECT_EQ(sample.time, time);
							            EXPECT_EQ(sample.data.size(), obsv.size * 2);
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
			        [&](PlasticityRule::TimedRecording::ObservablePerSynapse const& obsv) {
				        EXPECT_TRUE(timed_recording_data.data_per_synapse.contains(name));
				        auto const& data_entry_variant =
				            timed_recording_data.data_per_synapse.at(name);
				        std::visit(
				            [&](auto type) {
					            EXPECT_EQ(
					                data_entry_variant.size(), plasticity_rule.projections.size());
					            for (size_t p = 0; p < plasticity_rule.projections.size(); ++p) {
						            auto const& data_entry = std::get<std::vector<
						                grenade::vx::common::TimedDataSequence<std::vector<
						                    std::vector<typename decltype(type)::ElementType>>>>>(
						                data_entry_variant.at(plasticity_rule.projections.at(p)));
						            EXPECT_EQ(data_entry.size(), inputs.batch_size());
						            for (size_t i = 0; i < data_entry.size(); ++i) {
							            auto const& samples = data_entry.at(i);
							            EXPECT_EQ(
							                samples.size(), plasticity_rule.timer.num_periods);
							            for (size_t time = 0; auto const& sample : samples) {
								            EXPECT_EQ(sample.time, time);
								            EXPECT_EQ(
								                sample.data.size(),
								                network->projections
								                    .at(plasticity_rule.projections.at(p))
								                    .connections.size());
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
			        [&](PlasticityRule::TimedRecording::ObservablePerNeuron const& obsv) {
				        EXPECT_TRUE(timed_recording_data.data_per_neuron.contains(name));
				        auto const& data_entry_variant =
				            timed_recording_data.data_per_neuron.at(name);
				        std::visit(
				            [&](auto type) {
					            EXPECT_EQ(
					                data_entry_variant.size(), plasticity_rule.populations.size());
					            for (size_t p = 0; p < plasticity_rule.populations.size(); ++p) {
						            auto const& data_entry = std::get<std::vector<
						                grenade::vx::common::TimedDataSequence<std::vector<std::map<
						                    CompartmentOnLogicalNeuron,
						                    std::vector<typename decltype(type)::ElementType>>>>>>(
						                data_entry_variant.at(
						                    plasticity_rule.populations.at(p).descriptor));
						            EXPECT_EQ(data_entry.size(), inputs.batch_size());
						            for (size_t i = 0; i < data_entry.size(); ++i) {
							            auto const& samples = data_entry.at(i);
							            EXPECT_EQ(
							                samples.size(), plasticity_rule.timer.num_periods);
							            for (size_t time = 0; auto const& sample : samples) {
								            EXPECT_EQ(sample.time, time);
								            EXPECT_EQ(
								                sample.data.size(),
								                std::get<Population>(
								                    network->populations.at(
								                        plasticity_rule.populations.at(p)
								                            .descriptor))
								                    .neurons.size());
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
	using namespace grenade::vx;
	using namespace grenade::vx::network;

	signal_flow::ExecutionInstance instance;

	execution::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[instance] = lola::vx::v3::Chip();

	signal_flow::IODataMap inputs;
	inputs.runtime.push_back(
	    {{instance,
	      grenade::vx::common::Time(grenade::vx::common::Time::fpga_clock_cycles_per_us * 1000)}});

	// Construct connection to HW
	execution::JITGraphExecutor executor;

	Projection::Connection::Weight weight_63(63);
	Projection::Connection::Weight weight_32(32);

	auto const execute = [&](bool const set_weight) {
		NetworkBuilder network_builder;

		Population::Neurons neurons{Population::Neuron(
		    LogicalNeuronOnDLS(
		        LogicalNeuronCompartments(
		            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
		        AtomicNeuronOnDLS()),
		    Population::Neuron::Compartments{
		        {CompartmentOnLogicalNeuron(),
		         Population::Neuron::Compartment{
		             Population::Neuron::Compartment::SpikeMaster(0, false),
		             {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})};
		Population population{std::move(neurons)};
		auto const population_descriptor = network_builder.add(population);

		Projection::Connections projection_connections;
		for (size_t i = 0; i < population.neurons.size(); ++i) {
			projection_connections.push_back(
			    {{i, CompartmentOnLogicalNeuron()}, {i, CompartmentOnLogicalNeuron()}, weight_63});
		}
		Projection projection{
		    Receptor(Receptor::ID(), Receptor::Type::excitatory), std::move(projection_connections),
		    population_descriptor, population_descriptor};
		auto const projection_descriptor = network_builder.add(projection);

		PlasticityRule plasticity_rule;
		plasticity_rule.timer.start = PlasticityRule::Timer::Value(0);
		plasticity_rule.timer.period = PlasticityRule::Timer::Value(10000);
		plasticity_rule.timer.num_periods = 1;
		plasticity_rule.projections.push_back(projection_descriptor);
		plasticity_rule.recording = PlasticityRule::TimedRecording{
		    {{"w", PlasticityRule::TimedRecording::ObservablePerSynapse{
		               PlasticityRule::TimedRecording::ObservablePerSynapse::Type::int8,
		               PlasticityRule::TimedRecording::ObservablePerSynapse::LayoutPerRow::
		                   packed_active_columns}}}};

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
			kernel << "  w = " << static_cast<int>(weight_32.value()) << ";\n";
		}
		kernel << "  for (size_t i = 0; i < std::get<0>(recording.w).size(); ++i) {\n";
		kernel << "    size_t active_column = 0;\n";
		kernel << "    for (size_t j = 0; j < synapses[0].columns.size; ++j) {\n";
		kernel << "      if (synapses[0].columns.test(j)) {\n";
		kernel << "        std::get<0>(recording.w)[i][active_column] = w[j];\n";
		kernel << "        active_column++;\n";
		kernel << "      }\n";
		kernel << "    }\n";
		kernel << "  synapses[0].set_weights(w, 0);\n";
		kernel << "  }\n";
		kernel << "}\n";
		plasticity_rule.kernel = kernel.str();

		auto const plasticity_rule_descriptor = network_builder.add(plasticity_rule);

		auto const network = network_builder.done();
		auto const routing_result = routing::PortfolioRouter()(network);
		auto const network_graph = build_network_graph(network, routing_result);

		// run graph with given inputs and return results
		auto const result_map =
		    execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

		auto const result =
		    std::get<std::vector<common::TimedDataSequence<std::vector<std::vector<int8_t>>>>>(
		        std::get<PlasticityRule::TimedRecordingData>(
		            extract_plasticity_rule_recording_data(
		                result_map, network_graph, plasticity_rule_descriptor))
		            .data_per_synapse.at("w")
		            .at(projection_descriptor));
		assert(result.size() == inputs.batch_size());
		assert(result.at(0).size() == plasticity_rule.timer.num_periods);
		return static_cast<int>(result.at(0).at(0).data.at(0).at(0));
	};

	auto const with_weight_32 = execute(true);
	auto const with_weight_63 = execute(false);
	EXPECT_EQ(with_weight_32, weight_32);
	EXPECT_EQ(with_weight_63, weight_63);
}
