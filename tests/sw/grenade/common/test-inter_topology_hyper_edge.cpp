#include "grenade/common/inter_topology_hyper_edge.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <gtest/gtest.h>


using namespace grenade::common;

namespace grenade_common_inter_topology_hyper_edge_tests {

struct Dummy0VertexPortType : public EmptyVertexPortType<Dummy0VertexPortType>
{};

struct DummyDefaultVertex : public Vertex
{
	Port input_port;
	Port output_port;

	DummyDefaultVertex() = default;
	DummyDefaultVertex(Port in, Port out) : input_port(in), output_port(out) {}

	virtual std::vector<Port> get_input_ports() const override
	{
		return {input_port};
	}

	virtual std::vector<Port> get_output_ports() const override
	{
		return {output_port};
	}

	virtual std::unique_ptr<Vertex> copy() const override
	{
		return std::make_unique<DummyDefaultVertex>(*this);
	}

	virtual std::unique_ptr<Vertex> move() override
	{
		return std::make_unique<DummyDefaultVertex>(std::move(*this));
	}

	virtual bool is_equal_to(Vertex const& other) const override
	{
		return input_port == static_cast<DummyDefaultVertex const&>(other).input_port &&
		       output_port == static_cast<DummyDefaultVertex const&>(other).output_port;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};

} // namespace grenade_common_inter_topology_hyper_edge_tests

using namespace grenade_common_inter_topology_hyper_edge_tests;

TEST(InterTopologyHyperEdge, General)
{
	InterTopologyHyperEdge value;
	auto const empty_topology = std::make_shared<Topology>();
	LinkedTopology empty_linked_topology(empty_topology);
	Dummy0VertexPortType port_type;
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	Vertex::Port port_0(
	    port_type, Vertex::Port::SumOrSplitSupport::yes,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    Vertex::Port::RequiresOrGeneratesData::no, channels_0);

	DummyDefaultVertex vertex(port_0, port_0);
	auto const reference_vertex_descriptor = empty_topology->add_vertex(vertex);
	auto const link_vertex_descriptor = empty_linked_topology.add_vertex(vertex);

	EXPECT_TRUE((value.valid(
	    {link_vertex_descriptor}, {reference_vertex_descriptor}, empty_linked_topology)));

	EXPECT_EQ(
	    value.map_input_data({}, {link_vertex_descriptor}, {}, empty_linked_topology).size(), 1);
	EXPECT_FALSE(
	    value.map_input_data({}, {link_vertex_descriptor}, {}, empty_linked_topology).at(0).at(0));

	EXPECT_EQ(
	    value.map_output_data({}, {}, {reference_vertex_descriptor}, empty_linked_topology).size(),
	    1);
	EXPECT_FALSE(value.map_output_data({}, {}, {reference_vertex_descriptor}, empty_linked_topology)
	                 .at(0)
	                 .at(0));

	EXPECT_EQ(value, value);
	std::stringstream ss;
	ss << value;
	EXPECT_EQ(ss.str(), "InterTopologyHyperEdge()");
}
