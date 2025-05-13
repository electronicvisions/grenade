#include <gtest/gtest.h>

#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "grenade/vx/signal_flow/output_data.h"
#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/data_output.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "lola/vx/v3/chip.h"

TEST(Reconfiguration, CADC)
{
	using namespace grenade::vx::signal_flow;
	grenade::common::ExecutionInstanceID instance;
	Graph cadc_using_graph;
	vertex::NeuronView neuron(
	    vertex::NeuronView::Columns(1), vertex::NeuronView::Configs(1), vertex::NeuronView::Row());
	Graph::vertex_descriptor neuron_descriptor = cadc_using_graph.add(neuron, instance, {});
	vertex::CADCMembraneReadoutView cadc(
	    vertex::CADCMembraneReadoutView::Columns{
	        {halco::hicann_dls::vx::v3::SynapseOnSynapseRow{}}},
	    vertex::CADCMembraneReadoutView::Synram(), vertex::CADCMembraneReadoutView::Mode::periodic,
	    vertex::CADCMembraneReadoutView::Sources{{{}}});
	Graph::vertex_descriptor cadc_descriptor =
	    cadc_using_graph.add(cadc, instance, {neuron_descriptor});
	vertex::DataOutput data_output(ConnectionType::Int8, 1);
	Graph::vertex_descriptor output_descriptor =
	    cadc_using_graph.add(data_output, instance, {cadc_descriptor});

	Graph not_cadc_using_graph;
	not_cadc_using_graph.add(neuron, instance, {});

	grenade::vx::execution::JITGraphExecutor executor;
	InputData inputs;
	inputs.snippets.resize(5);
	for (auto& snippet : inputs.snippets) {
		snippet.runtime.push_back({{instance, grenade::vx::common::Time(10000)}});
	}

	grenade::vx::execution::JITGraphExecutor::ChipConfigs chip_configs;
	chip_configs[instance][grenade::vx::common::ChipOnConnection()] = lola::vx::v3::Chip();

	OutputData outputs = grenade::vx::execution::run(
	    executor,
	    {not_cadc_using_graph, cadc_using_graph, not_cadc_using_graph, cadc_using_graph,
	     cadc_using_graph},
	    {chip_configs, chip_configs, chip_configs, chip_configs, chip_configs}, inputs);

	EXPECT_TRUE(outputs.snippets[0].data.empty());
	EXPECT_GE(
	    std::get<std::vector<grenade::vx::common::TimedDataSequence<std::vector<Int8>>>>(
	        outputs.snippets[1].data.at(output_descriptor))
	        .at(0)
	        .size(),
	    0);
	EXPECT_TRUE(outputs.snippets[2].data.empty());
	EXPECT_GE(
	    std::get<std::vector<grenade::vx::common::TimedDataSequence<std::vector<Int8>>>>(
	        outputs.snippets[3].data.at(output_descriptor))
	        .at(0)
	        .size(),
	    0);
	EXPECT_GE(
	    std::get<std::vector<grenade::vx::common::TimedDataSequence<std::vector<Int8>>>>(
	        outputs.snippets[4].data.at(output_descriptor))
	        .at(0)
	        .size(),
	    0);
}
