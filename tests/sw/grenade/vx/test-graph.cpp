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

using namespace halco::hicann_dls::vx::v2;
using namespace grenade::vx;
using namespace grenade::vx::coordinate;
using namespace grenade::vx::vertex;

TEST(Graph, check_inputs_size)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataOutputInt8, 123);
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
	ExternalInput vertex(ConnectionType::DataOutputInt8, 123);
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
	ExternalInput vertex(ConnectionType::DataOutputInt8, 123);
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
	ExternalInput vertex(ConnectionType::DataOutputInt8, 123);
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
	ExternalInput vertex(ConnectionType::DataInputUInt16, 1);
	auto const v0 = graph.add(vertex, ExecutionInstance(), {});

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::CrossbarInputLabel, 1);
	auto const v1 = graph.add(vertex2, ExecutionInstance(), {v0});

	// connect loopback via crossbar
	CrossbarNode vertex3(
	    CrossbarNodeOnDLS(
	        SPL1Address().toCrossbarInputOnDLS(),
	        SPL1Address().toCrossbarL2OutputOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::v2::CrossbarNode());

	// Graph: v0 -> v1 -> v2
	auto const v2 = graph.add(vertex3, ExecutionInstance(), {v1});

	// Graph: v0 -> v1 -> v2 -> v3
	CrossbarL2Output vertex4;
	auto const v3 = graph.add(vertex4, ExecutionInstance(), {v2});

	DataOutput vertex5(ConnectionType::DataOutputUInt16, 1);
	EXPECT_NO_THROW(graph.add(vertex5, ExecutionInstance(), {v3}));

	// crossbar node not connecting loopback
	CrossbarNode vertex6(
	    CrossbarNodeOnDLS(CrossbarInputOnDLS(8), PADIBusOnDLS().toCrossbarOutputOnDLS()),
	    haldls::vx::v2::CrossbarNode());

	// Graph: v0 -> v1 -> v4
	auto const v4 = graph.add(vertex6, ExecutionInstance(), {v1});

	// Input from crossbar node not connecting to output not supported
	EXPECT_THROW(graph.add(vertex5, ExecutionInstance(), {v4}), std::runtime_error);
}
