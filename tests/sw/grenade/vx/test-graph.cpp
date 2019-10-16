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

TEST(Graph, General)
{
	Graph graph;

	// Graph: v0
	ExternalInput vertex(ConnectionType::DataOutputUInt5, 123);
	EXPECT_NO_THROW(graph.add(vertex, ExecutionInstance(), {}));

	// too much input vertices
	EXPECT_THROW(graph.add(vertex, ExecutionInstance(), {0}), std::runtime_error);

	// Graph: v0 -> v1
	DataInput vertex2(ConnectionType::SynapseInputLabel, 123);
	EXPECT_NO_THROW(graph.add(vertex2, ExecutionInstance(), {0}));

	// data type flow not allowed over different execution indices
	EXPECT_THROW(
	    graph.add(vertex2, ExecutionInstance(ExecutionIndex(1), HemisphereGlobal()), {0}),
	    std::runtime_error);

	// too little input vertices
	EXPECT_THROW(graph.add(vertex2, ExecutionInstance(), {}), std::runtime_error);
	// too much input vertices
	EXPECT_THROW(graph.add(vertex2, ExecutionInstance(), {0, 1}), std::runtime_error);

	// wrong port shape
	DataInput vertex3(ConnectionType::SynapseInputLabel, 42);
	EXPECT_THROW(graph.add(vertex3, ExecutionInstance(), {0}), std::runtime_error);

	// wrong port type
	EXPECT_THROW(graph.add(vertex2, ExecutionInstance(), {1}), std::runtime_error);

	// Graph:: v0 -> v1 -> v2
	DataOutput vertex4(ConnectionType::SynapseInputLabel, 123);
	EXPECT_NO_THROW(graph.add(vertex4, ExecutionInstance(), {1}));

	// DataOutput -> DataInput does not go forward in time
	EXPECT_THROW(graph.add(vertex2, ExecutionInstance(), {2}), std::runtime_error);

	// Graph: v0 -> v1 -> v2 ==> v3
	EXPECT_NO_THROW(
	    graph.add(vertex2, ExecutionInstance(ExecutionIndex(1), HemisphereGlobal()), {2}));

	// Graph: v0 -> v1 -> v2 ==> v3
	//                 -> v4
	SynapseArrayView vertex5(SynapseArrayView::synapse_rows_type(123));
	EXPECT_NO_THROW(graph.add(vertex5, ExecutionInstance(), {1}));

	// Graph: v0 -> v1 -> v2 ==> v3
	//                 -> v4 -> v5
	NeuronView vertex6({});
	EXPECT_NO_THROW(graph.add(vertex6, ExecutionInstance(), {4}));

	// SynapseArrayView only allows single out edge
	EXPECT_THROW(graph.add(vertex6, ExecutionInstance(), {4}), std::runtime_error);
}
