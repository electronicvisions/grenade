#include <gtest/gtest.h>

#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/vertex/crossbar_l2_output.h"
#include "grenade/vx/vertex/crossbar_node.h"
#include "grenade/vx/vertex/data_input.h"
#include "grenade/vx/vertex/data_output.h"
#include "grenade/vx/vertex/external_input.h"
#include "grenade/vx/vertex/neuron_view.h"
#include "grenade/vx/vertex/synapse_array_view.h"
#include "halco/hicann-dls/vx/v2/event.h"

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>

using namespace halco::hicann_dls::vx::v2;
using namespace grenade::vx;
using namespace grenade::vx::coordinate;
using namespace grenade::vx::vertex;

TEST(Graph, check_inputs_size)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataInt8, 123);
	auto const v0 = graph.add(vertex, ExecutionInstance(), {});

	// too much input vertices
	EXPECT_THROW(graph.add(vertex, ExecutionInstance(), {v0}), std::runtime_error);

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::Int8, 123);
	auto const v1 = graph.add(vertex2, ExecutionInstance(), {v0});

	// too little input vertices
	EXPECT_THROW(graph.add(vertex2, ExecutionInstance(), {}), std::runtime_error);

	// too much input vertices
	EXPECT_THROW(graph.add(vertex2, ExecutionInstance(), {v0, v1}), std::runtime_error);
}


TEST(Graph, check_input_port)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataInt8, 123);
	auto const v0 = graph.add(vertex, ExecutionInstance(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::Int8, 123);
	auto const v1 = graph.add(vertex2, ExecutionInstance(), {v0});

	// wrong port shape
	DataInput vertex3(ConnectionType::Int8, 42);
	EXPECT_THROW(graph.add(vertex3, ExecutionInstance(), {v0}), std::runtime_error);

	// invalid port restriction
	EXPECT_THROW(
	    graph.add(vertex3, ExecutionInstance(), {{v0, {100, 141} /*size=42 but out-of-range*/}}),
	    std::runtime_error);

	// port restriction not matching input port of vertex (wrong size)
	EXPECT_THROW(
	    graph.add(vertex3, ExecutionInstance(), {{v0, {10, 52} /*arbitrary offset, size=43*/}}),
	    std::runtime_error);

	// correct port restriction
	EXPECT_NO_THROW(
	    graph.add(vertex3, ExecutionInstance(), {{v0, {10, 51} /*arbitrary offset, size=42*/}}));

	// wrong port type
	EXPECT_THROW(graph.add(vertex2, ExecutionInstance(), {v1}), std::runtime_error);
}

TEST(Graph, check_execution_instances)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataInt8, 123);
	auto const v0 = graph.add(vertex, ExecutionInstance(), {});

	// too much input vertices
	EXPECT_THROW(graph.add(vertex, ExecutionInstance(), {v0}), std::runtime_error);

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::Int8, 123);
	auto const v1 = graph.add(vertex2, ExecutionInstance(), {v0});

	// ExternalInput -> DataInput not allowed over different execution instances
	EXPECT_THROW(
	    graph.add(vertex2, ExecutionInstance(ExecutionIndex(1), DLSGlobal()), {v0}),
	    std::runtime_error);

	// Graph:: v0 -> v1 -> v2
	DataOutput vertex4(ConnectionType::Int8, 123);
	auto const v2 = graph.add(vertex4, ExecutionInstance(), {v1});

	// DataOutput -> DataInput allowed over differrent execution instances
	EXPECT_NO_THROW(graph.add(vertex2, ExecutionInstance(ExecutionIndex(1), DLSGlobal()), {v2}));

	// DataOutput -> DataInput only allowed over differrent execution instances
	EXPECT_THROW(graph.add(vertex2, ExecutionInstance(), {v2}), std::runtime_error);
}

void test_check_acyclicity(bool enable_check)
{
	Graph graph(enable_check);

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataInt8, 123);
	auto const v0 = graph.add(vertex, ExecutionInstance(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::Int8, 123);
	auto const v1 = graph.add(vertex2, ExecutionInstance(), {v0});

	// Graph: v0 -> v1 -> v2
	DataOutput vertex4(ConnectionType::Int8, 123);
	auto const v2 = graph.add(vertex4, ExecutionInstance(), {v1});

	// Graph: v0 -> v1 -> v2 ==> v3
	auto const v3 = graph.add(vertex2, ExecutionInstance(ExecutionIndex(1), DLSGlobal()), {v2});

	// Graph: v0 -> v1 -> v2 ==> v3 -> v4
	auto const v4 = graph.add(vertex4, ExecutionInstance(ExecutionIndex(1), DLSGlobal()), {v3});

	// DataOutput -> DataInput back to ExecutionIndex(0) leads to cyclicity
	if (enable_check) {
		EXPECT_THROW(graph.add(vertex2, ExecutionInstance(), {v4}), std::runtime_error);
	} else {
		// Note that the graph still won't be executable and the executor will raise the exact same
		// exception.
		EXPECT_NO_THROW(graph.add(vertex2, ExecutionInstance(), {v4}));
	}
}

TEST(Graph, check_acyclic_execution_instances)

{
	test_check_acyclicity(true);
	test_check_acyclicity(false);
}


TEST(Graph, check_supports_input_from)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataTimedSpikeSequence, 1);
	auto const v0 = graph.add(vertex, ExecutionInstance(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::TimedSpikeSequence, 1);
	auto const v1 = graph.add(vertex2, ExecutionInstance(), {v0});

	// Graph: v0 -> v1 -> v2
	CrossbarL2Input vertex3;
	auto const v2 = graph.add(vertex3, ExecutionInstance(), {v1});

	// connect loopback via crossbar
	CrossbarNode vertex4(
	    CrossbarNodeOnDLS(
	        SPL1Address().toCrossbarInputOnDLS(),
	        SPL1Address().toCrossbarL2OutputOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::v2::CrossbarNode());

	// Graph: v0 -> v1 -> v2 -> v3
	auto const v3 = graph.add(vertex4, ExecutionInstance(), {v2});

	// Graph: v0 -> v1 -> v2 -> v3 -> v4
	CrossbarL2Output vertex5;
	auto const v4 = graph.add(vertex5, ExecutionInstance(), {v3});

	DataOutput vertex6(ConnectionType::TimedSpikeFromChipSequence, 1);
	EXPECT_NO_THROW(graph.add(vertex6, ExecutionInstance(), {v4}));

	// crossbar node not connecting loopback
	CrossbarNode vertex7(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), PADIBusOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::v2::CrossbarNode());

	// Graph: v0 -> v1 -> v5
	auto const v5 = graph.add(vertex7, ExecutionInstance(), {v2});

	// Input from crossbar node not connecting to output not supported
	EXPECT_THROW(graph.add(vertex6, ExecutionInstance(), {v5}), std::runtime_error);
}

TEST(Graph, recurrence)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataTimedSpikeSequence, 1);
	auto const v0 = graph.add(vertex, ExecutionInstance(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::TimedSpikeSequence, 1);
	auto const v1 = graph.add(vertex2, ExecutionInstance(), {v0});

	// Graph: v1 -> v2
	CrossbarL2Input vertex3;
	auto const v2 = graph.add(vertex3, ExecutionInstance(), {v1});

	CrossbarNode crossbar_in(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), CrossbarOutputOnDLS(0)),
	    haldls::vx::v2::CrossbarNode());
	auto const v3 = graph.add(crossbar_in, ExecutionInstance(), {v2});

	PADIBus::Coordinate c;
	PADIBus padi_bus(c);
	auto const v4 = graph.add(padi_bus, ExecutionInstance(), {v3});

	SynapseDriver synapse_driver(
	    SynapseDriver::Coordinate(), SynapseDriver::Config(),
	    {haldls::vx::v2::SynapseDriverConfig::RowMode::excitatory,
	     haldls::vx::v2::SynapseDriverConfig::RowMode::excitatory});
	auto const v5 = graph.add(synapse_driver, ExecutionInstance(), {v4});

	SynapseArrayView synapses(
	    SynramOnDLS(), SynapseArrayView::Rows{SynapseRowOnDLS()},
	    SynapseArrayView::Columns{SynapseOnSynapseRow()},
	    SynapseArrayView::Weights{{lola::vx::v2::SynapseMatrix::Weight()}},
	    SynapseArrayView::Labels{{lola::vx::v2::SynapseMatrix::Label()}});
	auto const v6 = graph.add(synapses, ExecutionInstance(), {v5});

	NeuronView neurons(
	    NeuronView::Columns{NeuronColumnOnDLS()}, NeuronView::EnableResets{true}, NeuronRowOnDLS());
	auto const v7 = graph.add(neurons, ExecutionInstance(), {v6});

	NeuronEventOutputView neuron_outputs(
	    NeuronEventOutputView::Columns{NeuronColumnOnDLS()}, NeuronRowOnDLS());
	auto const v8 = graph.add(neuron_outputs, ExecutionInstance(), {v7});

	// recurrence
	CrossbarNode crossbar_recurrent(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(0), CrossbarOutputOnDLS(0)),
	    haldls::vx::v2::CrossbarNode());
	auto const v9 = graph.add(crossbar_recurrent, ExecutionInstance(), {v8});

	auto const v10 = graph.add(v4, ExecutionInstance(), {v3, v9});

	auto const v11 = graph.add(v5, ExecutionInstance(), {v10});

	auto const v12 = graph.add(v6, ExecutionInstance(), {v11});

	auto const v13 = graph.add(v7, ExecutionInstance(), {v12});

	auto const v14 = graph.add(v8, ExecutionInstance(), {v13});

	CrossbarNode crossbar_out(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(0), CrossbarL2OutputOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::v2::CrossbarNode());
	auto const v15 = graph.add(crossbar_out, ExecutionInstance(), {v14});

	CrossbarL2Output crossbar_l2_output;
	auto const v16 = graph.add(crossbar_l2_output, ExecutionInstance(), {v15});

	DataOutput data_output(ConnectionType::TimedSpikeFromChipSequence, 1);
	graph.add(data_output, ExecutionInstance(), {v16});

	// assignment, construction
	Graph graph2(graph); // copy construct
	EXPECT_EQ(graph2, graph);

	Graph graph3;
	graph3 = graph; // copy assign
	EXPECT_EQ(graph2, graph);

	Graph graph4(std::move(graph)); // move construct
	EXPECT_EQ(graph4, graph3);

	Graph graph5;
	graph5 = std::move(graph4); // move assign
	EXPECT_EQ(graph5, graph3);
}

TEST(Graph, CerealizeCoverage)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataTimedSpikeSequence, 1);
	auto const v0 = graph.add(vertex, ExecutionInstance(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::TimedSpikeSequence, 1);
	auto const v1 = graph.add(vertex2, ExecutionInstance(), {v0});

	// Graph: v1 -> v2
	CrossbarL2Input vertex3;
	auto const v2 = graph.add(vertex3, ExecutionInstance(), {v1});

	CrossbarNode crossbar_in(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), CrossbarOutputOnDLS(0)),
	    haldls::vx::v2::CrossbarNode());
	auto const v3 = graph.add(crossbar_in, ExecutionInstance(), {v2});

	PADIBus::Coordinate c;
	PADIBus padi_bus(c);
	auto const v4 = graph.add(padi_bus, ExecutionInstance(), {v3});

	SynapseDriver synapse_driver(
	    SynapseDriver::Coordinate(), SynapseDriver::Config(),
	    {haldls::vx::v2::SynapseDriverConfig::RowMode::excitatory,
	     haldls::vx::v2::SynapseDriverConfig::RowMode::excitatory});
	auto const v5 = graph.add(synapse_driver, ExecutionInstance(), {v4});

	SynapseArrayView synapses(
	    SynramOnDLS(), SynapseArrayView::Rows{SynapseRowOnDLS()},
	    SynapseArrayView::Columns{SynapseOnSynapseRow()},
	    SynapseArrayView::Weights{{lola::vx::v2::SynapseMatrix::Weight()}},
	    SynapseArrayView::Labels{{lola::vx::v2::SynapseMatrix::Label()}});
	auto const v6 = graph.add(synapses, ExecutionInstance(), {v5});

	NeuronView neurons(
	    NeuronView::Columns{NeuronColumnOnDLS()}, NeuronView::EnableResets{true}, NeuronRowOnDLS());
	auto const v7 = graph.add(neurons, ExecutionInstance(), {v6});

	NeuronEventOutputView neuron_outputs(
	    NeuronEventOutputView::Columns{NeuronColumnOnDLS()}, NeuronRowOnDLS());
	auto const v8 = graph.add(neuron_outputs, ExecutionInstance(), {v7});

	// recurrence
	CrossbarNode crossbar_recurrent(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(0), CrossbarOutputOnDLS(0)),
	    haldls::vx::CrossbarNode());
	auto const v9 = graph.add(crossbar_recurrent, ExecutionInstance(), {v8});

	auto const v10 = graph.add(v4, ExecutionInstance(), {v3, v9});

	auto const v11 = graph.add(v5, ExecutionInstance(), {v10});

	auto const v12 = graph.add(v6, ExecutionInstance(), {v11});

	auto const v13 = graph.add(v7, ExecutionInstance(), {v12});

	auto const v14 = graph.add(v8, ExecutionInstance(), {v13});

	CrossbarNode crossbar_out(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(0), CrossbarL2OutputOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::CrossbarNode());
	auto const v15 = graph.add(crossbar_out, ExecutionInstance(), {v14});

	CrossbarL2Output crossbar_l2_output;
	auto const v16 = graph.add(crossbar_l2_output, ExecutionInstance(), {v15});

	DataOutput data_output(ConnectionType::TimedSpikeFromChipSequence, 1);
	graph.add(data_output, ExecutionInstance(), {v16});

	Graph graph2;

	std::ostringstream ostream;
	{
		cereal::JSONOutputArchive oa(ostream);
		oa(graph);
	}
	std::istringstream istream(ostream.str());
	{
		cereal::JSONInputArchive ia(istream);
		ia(graph2);
	}
	ASSERT_EQ(graph, graph2);
}
