#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/backend/run.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/placed_atomic/build_routing.h"
#include "grenade/vx/network/placed_atomic/cadc_recording.h"
#include "grenade/vx/network/placed_atomic/network.h"
#include "grenade/vx/network/placed_atomic/network_builder.h"
#include "grenade/vx/network/placed_atomic/network_graph.h"
#include "grenade/vx/network/placed_atomic/network_graph_builder.h"
#include "grenade/vx/network/placed_atomic/population.h"
#include "grenade/vx/network/placed_atomic/projection.h"
#include "grenade/vx/signal_flow/execution_instance.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/timer.h"
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

inline std::pair<lola::vx::v3::Chip, grenade::vx::execution::backend::Connection>
initialize_excitatory_bypass()
{
	std::unique_ptr<lola::vx::v3::Chip> chip = std::make_unique<lola::vx::v3::Chip>();
	// Initialize chip
	ExperimentInit init;
	for (auto const c : iter_all<CommonNeuronBackendConfigOnDLS>()) {
		chip->neuron_block.backends[c].set_enable_clocks(true);
	}
	for (auto const c : iter_all<ColumnCurrentQuadOnDLS>()) {
		for (auto const e : iter_all<EntryOnQuad>()) {
			auto s = init.column_current_quad_config[c].get_switch(e);
			s.set_enable_synaptic_current_excitatory(true);
			s.set_enable_synaptic_current_inhibitory(true);
			init.column_current_quad_config[c].set_switch(e, s);
		}
	}
	grenade::vx::execution::backend::Connection connection(
	    hxcomm::vx::get_connection_from_env(), init);
	stadls::vx::v3::PlaybackProgramBuilder builder;
	// enable excitatory bypass mode
	for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
		auto& config = chip->neuron_block.atomic_neurons[neuron];
		config.event_routing.enable_digital = true;
		config.event_routing.analog_output =
		    lola::vx::v3::AtomicNeuron::EventRouting::AnalogOutputMode::normal;
		config.event_routing.enable_bypass_excitatory = true;
		config.threshold.enable = false;
	}
	for (auto const block : iter_all<CommonPADIBusConfigOnDLS>()) {
		auto& padi_config = chip->synapse_driver_blocks[block.toSynapseDriverBlockOnDLS()].padi_bus;
		auto dacen = padi_config.get_dacen_pulse_extension();
		for (auto const block : iter_all<PADIBusOnPADIBusBlock>()) {
			dacen[block] = CommonPADIBusConfig::DacenPulseExtension(15);
		}
		padi_config.set_dacen_pulse_extension(dacen);
	}
	for (auto const drv : iter_all<SynapseDriverOnDLS>()) {
		auto& config = chip->synapse_driver_blocks[drv.toSynapseDriverBlockOnDLS()]
		                   .synapse_drivers[drv.toSynapseDriverOnSynapseDriverBlock()];
		config.set_enable_receiver(true);
		config.set_enable_address_out(true);
	}
	for (auto const block : iter_all<SynapseBlockOnDLS>()) {
		chip->synapse_blocks[block].i_bias_dac.fill(CapMemCell::Value(1022));
	}
	auto program = builder.done();
	grenade::vx::execution::backend::run(connection, program);
	return std::make_pair(*chip, std::move(connection));
}

TEST(CADCRecording, General)
{
	// Construct connection to HW
	auto [chip_config, connection] = initialize_excitatory_bypass();
	std::map<DLSGlobal, grenade::vx::execution::backend::Connection> connections;
	connections.emplace(DLSGlobal(), std::move(connection));
	grenade::vx::execution::JITGraphExecutor executor(std::move(connections));

	grenade::vx::signal_flow::ExecutionInstance instance;

	grenade::vx::execution::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[instance] = chip_config;

	// build network
	grenade::vx::network::placed_atomic::NetworkBuilder network_builder;

	grenade::vx::network::placed_atomic::Population::Neurons neurons;
	grenade::vx::network::placed_atomic::Population::EnableRecordSpikes enable_record_spikes;
	for (auto const column : iter_all<NeuronColumnOnDLS>()) {
		neurons.push_back(AtomicNeuronOnDLS(column, NeuronRowOnDLS::top));
		enable_record_spikes.push_back(false);
	}
	std::mt19937 rng{std::random_device{}()};
	std::shuffle(neurons.begin(), neurons.end(), rng);
	grenade::vx::network::placed_atomic::Population population_internal{
	    std::move(neurons), enable_record_spikes};
	auto const population_internal_descriptor = network_builder.add(population_internal);

	grenade::vx::network::placed_atomic::CADCRecording cadc_recording;
	cadc_recording.neurons.push_back(grenade::vx::network::placed_atomic::CADCRecording::Neuron(
	    population_internal_descriptor, 14,
	    grenade::vx::network::placed_atomic::CADCRecording::Neuron::Source::membrane));
	cadc_recording.neurons.push_back(grenade::vx::network::placed_atomic::CADCRecording::Neuron(
	    population_internal_descriptor, 60,
	    grenade::vx::network::placed_atomic::CADCRecording::Neuron::Source::membrane));
	cadc_recording.neurons.push_back(grenade::vx::network::placed_atomic::CADCRecording::Neuron(
	    population_internal_descriptor, 25,
	    grenade::vx::network::placed_atomic::CADCRecording::Neuron::Source::membrane));
	cadc_recording.neurons.push_back(grenade::vx::network::placed_atomic::CADCRecording::Neuron(
	    population_internal_descriptor, 150,
	    grenade::vx::network::placed_atomic::CADCRecording::Neuron::Source::membrane));
	network_builder.add(cadc_recording);

	auto const network = network_builder.done();

	auto const routing_result = grenade::vx::network::placed_atomic::build_routing(network);
	auto const network_graph =
	    grenade::vx::network::placed_atomic::build_network_graph(network, routing_result);

	grenade::vx::signal_flow::IODataMap inputs;
	inputs.runtime.push_back(
	    {{instance,
	      grenade::vx::common::Time(grenade::vx::common::Time::fpga_clock_cycles_per_us * 100)}});
	inputs.runtime.push_back(
	    {{instance,
	      grenade::vx::common::Time(grenade::vx::common::Time::fpga_clock_cycles_per_us * 150)}});

	// run graph with given inputs and return results
	auto const result_map =
	    grenade::vx::execution::run(executor, network_graph.get_graph(), inputs, chip_configs);

	assert(network_graph.get_cadc_sample_output_vertex().size());
	EXPECT_EQ(network_graph.get_cadc_sample_output_vertex().size(), 1);
	auto const result = std::get<std::vector<
	    grenade::vx::common::TimedDataSequence<std::vector<grenade::vx::signal_flow::Int8>>>>(
	    result_map.data.at(network_graph.get_cadc_sample_output_vertex().at(0)));

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
