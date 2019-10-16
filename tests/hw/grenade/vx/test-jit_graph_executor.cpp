#include <gtest/gtest.h>

#include "grenade/vx/config.h"
#include "grenade/vx/data_map.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/chip.h"
#include "haldls/vx/systime.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
#include "stadls/vx/init_generator.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/run.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx;
using namespace stadls::vx;
using namespace lola::vx;
using namespace haldls::vx;

TEST(JITGraphExecutor, SingleMAC)
{
	logger_default_config(log4cxx::Level::getTrace());

	constexpr size_t size = 256;

	grenade::vx::vertex::ExternalInput external_input(
	    grenade::vx::ConnectionType::DataOutputUInt5, size);

	grenade::vx::vertex::DataInput data_input(grenade::vx::ConnectionType::SynapseInputLabel, size);

	// construct SynapseArrayView on one vertical half with size 256 (x) * 256 (y)
	grenade::vx::vertex::SynapseArrayView::synapse_rows_type synapse_rows;
	// fill synapse array view
	for (auto synapse_row : iter_all<SynapseRowOnSynram>()) {
		// all synapse rows
		synapse_rows.push_back(grenade::vx::vertex::SynapseArrayView::SynapseRow());
		synapse_rows.back().coordinate = synapse_row;
		synapse_rows.back().weights = std::vector<SynapseRow::Weight>(size);
		// even rows inhibitory, odd rows excitatory
		synapse_rows.back().mode = synapse_row.toEnum() % 2
		                               ? SynapseDriverConfig::RowMode::excitatory
		                               : SynapseDriverConfig::RowMode::inhibitory;
	}

	// construct vertex::SynapseArrayView
	grenade::vx::vertex::SynapseArrayView synapses(synapse_rows);

	grenade::vx::vertex::NeuronView::neurons_type nrns;
	for (auto nrn : iter_all<NeuronColumnOnDLS>()) {
		nrns.push_back(nrn);
	}
	grenade::vx::vertex::NeuronView neurons(nrns);

	grenade::vx::vertex::CADCMembraneReadoutView readout(size);

	grenade::vx::vertex::ConvertInt8ToSynapseInputLabel convert(size);

	grenade::vx::vertex::DataOutput data_output(
	    grenade::vx::ConnectionType::SynapseInputLabel, size);

	grenade::vx::Graph g;

	grenade::vx::coordinate::ExecutionInstance instance(
	    grenade::vx::coordinate::ExecutionIndex(0), HemisphereGlobal());

	auto const v1 = g.add(external_input, instance, {});
	auto const v2 = g.add(data_input, instance, {v1});
	auto const v3 = g.add(synapses, instance, {v2});
	auto const v4 = g.add(neurons, instance, {v3});
	auto const v5 = g.add(readout, instance, {v4});
	auto const v6 = g.add(convert, instance, {v5});
	auto const v7 = g.add(data_output, instance, {v6});

	// Construct map of one executor and connect to HW
	grenade::vx::JITGraphExecutor::ExecutorMap executors;
	auto connection = hxcomm::vx::get_connection_from_env();
	executors.insert(std::pair<DLSGlobal, hxcomm::vx::ConnectionVariant&>(DLSGlobal(), connection));

	// Initialize chip
	{
		DigitalInit const init;
		auto [builder, _] = generate(init);
		auto program = builder.done();
		stadls::vx::run(executors.at(DLSGlobal()), program);
	}

	// fill graph inputs (with UInt5(0))
	grenade::vx::DataMap input_list;
	std::vector<grenade::vx::UInt5> inputs(size);
	input_list.uint5.insert(std::make_pair(v1, inputs));

	grenade::vx::JITGraphExecutor::ConfigMap config_map;
	std::unique_ptr<grenade::vx::ChipConfig> chip = std::make_unique<grenade::vx::ChipConfig>();
	config_map[DLSGlobal()] = *chip;

	// run Graph with given inputs and return results
	auto const output_activation_map =
	    grenade::vx::JITGraphExecutor::run(g, input_list, executors, config_map);

	EXPECT_EQ(output_activation_map.uint5.size(), 1);

	EXPECT_TRUE(output_activation_map.uint5.find(v7) != output_activation_map.uint5.end());
	EXPECT_EQ(output_activation_map.uint5.at(v7).size(), size);
}
