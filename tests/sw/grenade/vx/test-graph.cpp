#include <gtest/gtest.h>

#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/vertex/data_input.h"
#include "grenade/vx/vertex/data_output.h"
#include "grenade/vx/vertex/external_input.h"
#include "grenade/vx/vertex/neuron_view.h"
#include "grenade/vx/vertex/synapse_array_view.h"

using namespace halco::hicann_dls::vx;
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
