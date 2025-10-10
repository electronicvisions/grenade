#include "grenade/common/topology.h"

#include "grenade/common/edge.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/vertex.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <stdexcept>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace grenade_common_topology_tests {

struct DummyVertexPortType : public EmptyVertexPortType<DummyVertexPortType>
{
private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::base_class<Vertex::Port::Type>(this));
	}
};


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


private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::base_class<Vertex>(this));
		ar(input_port);
		ar(output_port);
	}
};

} // namespace grenade_common_topology_tests

using namespace grenade_common_topology_tests;


TEST(Topology, General)
{
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	DummyVertexPortType port_type_0;

	Vertex::Port port_0(
	    port_type_0, Vertex::Port::SumOrSplitSupport::no,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    Vertex::Port::RequiresOrGeneratesData::no, channels_0);

	DummyDefaultVertex default_vertex{port_0, port_0};

	Topology topology;

	auto const vertex_0_descr = topology.add_vertex(default_vertex);
	auto const vertex_1_descr = topology.add_vertex(default_vertex);
	topology.add_edge(
	    vertex_0_descr, vertex_1_descr,
	    Edge(ListMultiIndexSequence({MultiIndex({1})}), ListMultiIndexSequence({MultiIndex({1})})));

	// invalid edge from source
	EXPECT_THROW(
	    (topology.add_edge(
	        vertex_0_descr, vertex_1_descr,
	        Edge(
	            ListMultiIndexSequence({MultiIndex({3})}),
	            ListMultiIndexSequence({MultiIndex({1})})))),
	    std::invalid_argument);

	// invalid edge to target
	EXPECT_THROW(
	    (topology.add_edge(
	        vertex_0_descr, vertex_1_descr,
	        Edge(
	            ListMultiIndexSequence({MultiIndex({1})}),
	            ListMultiIndexSequence({MultiIndex({3})})))),
	    std::invalid_argument);

	// no source vertex split support
	EXPECT_THROW(
	    (topology.add_edge(
	        vertex_0_descr, vertex_1_descr,
	        Edge(
	            ListMultiIndexSequence({MultiIndex({1})}),
	            ListMultiIndexSequence({MultiIndex({2})})))),
	    std::invalid_argument);

	// no target vertex sum support
	EXPECT_THROW(
	    (topology.add_edge(
	        vertex_0_descr, vertex_1_descr,
	        Edge(
	            ListMultiIndexSequence({MultiIndex({2})}),
	            ListMultiIndexSequence({MultiIndex({1})})))),
	    std::invalid_argument);

	EXPECT_TRUE(topology.valid_strong_components());
	EXPECT_TRUE(topology.valid());
	EXPECT_EQ(topology, topology);
	EXPECT_EQ(topology, Topology(topology));

	topology.add_edge(
	    vertex_1_descr, vertex_0_descr,
	    Edge(ListMultiIndexSequence({MultiIndex({1})}), ListMultiIndexSequence({MultiIndex({1})})));
	EXPECT_FALSE(topology.valid_strong_components());
	EXPECT_FALSE(topology.valid());
}

CEREAL_REGISTER_TYPE(grenade_common_topology_tests::DummyVertexPortType)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::Vertex::Port::Type, grenade_common_topology_tests::DummyVertexPortType)
CEREAL_REGISTER_TYPE(grenade_common_topology_tests::DummyDefaultVertex)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::Vertex, grenade_common_topology_tests::DummyDefaultVertex)

TEST(Topology, Cerealization)
{
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	DummyVertexPortType port_type_0;

	Vertex::Port port_0(
	    port_type_0, Vertex::Port::SumOrSplitSupport::no,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    Vertex::Port::RequiresOrGeneratesData::no, channels_0);

	DummyDefaultVertex default_vertex{port_0, port_0};

	Topology obj;

	auto const vertex_0_descr = obj.add_vertex(default_vertex);
	auto const vertex_1_descr = obj.add_vertex(default_vertex);
	obj.add_edge(
	    vertex_0_descr, vertex_1_descr,
	    Edge(ListMultiIndexSequence({MultiIndex({1})}), ListMultiIndexSequence({MultiIndex({1})})));

	Topology obj2;

	std::ostringstream ostream;
	{
		cereal::JSONOutputArchive oa(ostream);
		oa(obj);
	}

	std::istringstream istream(ostream.str());
	{
		cereal::JSONInputArchive ia(istream);
		ia(obj2);
	}

	ASSERT_EQ(obj2, obj);
}
