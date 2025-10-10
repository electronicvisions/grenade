#include "grenade/common/vertex.h"

#include "grenade/common/edge.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <stdexcept>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace grenade_common_vertex_tests {

struct Dummy0VertexPortType : public EmptyVertexPortType<Dummy0VertexPortType>
{
private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::base_class<Vertex::Port::Type>(this));
	}
};

struct Dummy1VertexPortType : public EmptyVertexPortType<Dummy1VertexPortType>
{
private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::base_class<Vertex::Port::Type>(this));
	}
};

} // namespace grenade_common_vertex_tests

using namespace grenade_common_vertex_tests;

TEST(Vertex, Port)
{
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	ListMultiIndexSequence channels_1({MultiIndex({2}), MultiIndex({2})});
	ListMultiIndexSequence channels_2({MultiIndex({2}), MultiIndex({3})});
	Dummy0VertexPortType port_type_0;
	Dummy1VertexPortType port_type_1;

	Vertex::Port port_0(
	    port_type_0, Vertex::Port::SumOrSplitSupport::yes,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    Vertex::Port::RequiresOrGeneratesData::yes, channels_0);
	EXPECT_EQ(port_0.get_type(), port_type_0);
	EXPECT_EQ(port_0.sum_or_split_support, Vertex::Port::SumOrSplitSupport::yes);
	EXPECT_EQ(
	    port_0.execution_instance_transition_constraint,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required);
	EXPECT_EQ(port_0.get_channels(), channels_0);
	EXPECT_EQ(
	    (Vertex::Port(
	        port_type_0, Vertex::Port::SumOrSplitSupport::yes,
	        Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	        Vertex::Port::RequiresOrGeneratesData::yes, std::move(*channels_0.copy()))),
	    port_0);

	// not injective
	EXPECT_THROW(port_0.set_channels(channels_1), std::invalid_argument);
	EXPECT_THROW(port_0.set_channels(std::move(*channels_1.copy())), std::invalid_argument);
	EXPECT_THROW(
	    (Vertex::Port(
	        port_type_0, Vertex::Port::SumOrSplitSupport::yes,
	        Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	        Vertex::Port::RequiresOrGeneratesData::yes, channels_1)),
	    std::invalid_argument);
	EXPECT_THROW(
	    (Vertex::Port(
	        port_type_0, Vertex::Port::SumOrSplitSupport::yes,
	        Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	        Vertex::Port::RequiresOrGeneratesData::yes, std::move(*channels_1.copy()))),
	    std::invalid_argument);

	EXPECT_NO_THROW(port_0.set_channels(channels_2));
	EXPECT_EQ(port_0.get_channels(), channels_2);
	EXPECT_NO_THROW(port_0.set_channels(std::move(*channels_0.copy())));
	EXPECT_EQ(port_0.get_channels(), channels_0);

	auto port_0_copy = port_0;
	EXPECT_EQ(port_0_copy, port_0);
	auto port_0_move = std::move(port_0_copy);
	EXPECT_EQ(port_0_move, port_0);
	port_0_move.set_type(port_type_1);
	EXPECT_NE(port_0_move, port_0);
	port_0_move.set_type(port_type_0);
	port_0_move.sum_or_split_support = Vertex::Port::SumOrSplitSupport::no;
	EXPECT_NE(port_0_move, port_0);
	port_0_move.sum_or_split_support = Vertex::Port::SumOrSplitSupport::yes;
	port_0_move.execution_instance_transition_constraint =
	    Vertex::Port::ExecutionInstanceTransitionConstraint::supported;
	EXPECT_NE(port_0_move, port_0);
	port_0_move.execution_instance_transition_constraint =
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required;
	port_0_move.set_channels(channels_2);
	EXPECT_NE(port_0_move, port_0);
}


namespace grenade_common_vertex_tests {

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

} // namespace grenade_common_vertex_tests

TEST(Vertex, Default)
{
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	Dummy0VertexPortType port_type_0;
	Dummy1VertexPortType port_type_1;

	Vertex::Port port_0(
	    port_type_0, Vertex::Port::SumOrSplitSupport::yes,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    Vertex::Port::RequiresOrGeneratesData::no, channels_0);

	DummyDefaultVertex default_vertex{port_0, port_0};

	auto strong_component_invariant = default_vertex.get_strong_component_invariant();
	assert(strong_component_invariant);
	EXPECT_NE(*strong_component_invariant, *strong_component_invariant);

	ListMultiIndexSequence channels_1({MultiIndex({3}), MultiIndex({4})});

	Vertex::Port port_1(
	    port_type_0, Vertex::Port::SumOrSplitSupport::yes,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    Vertex::Port::RequiresOrGeneratesData::no, channels_0);

	DummyDefaultVertex default_vertex_1{port_1, port_1};

	// target port index out of range
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex, Edge(ListMultiIndexSequence(), ListMultiIndexSequence(), 0, 1))));
	// source port index out of range
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex, Edge(ListMultiIndexSequence(), ListMultiIndexSequence(), 1, 0))));
	// port types don't match
	default_vertex_1.output_port.set_type(port_type_1);
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex_1, Edge(ListMultiIndexSequence(), ListMultiIndexSequence(), 0, 0))));
	default_vertex_1.output_port.set_type(port_type_0);
	// execution instance transition constraint combination
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex_1, Edge(ListMultiIndexSequence(), ListMultiIndexSequence()))));
	default_vertex.input_port.execution_instance_transition_constraint =
	    Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported;
	// channels not within input port
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex_1, Edge(
	                          ListMultiIndexSequence({MultiIndex({3})}),
	                          ListMultiIndexSequence({MultiIndex({3})})))));

	EXPECT_TRUE((default_vertex.valid_edge_from(
	    default_vertex_1, Edge(
	                          ListMultiIndexSequence({MultiIndex({1})}),
	                          ListMultiIndexSequence({MultiIndex({1})})))));

	// source port index out of range
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex, Edge(ListMultiIndexSequence(), ListMultiIndexSequence(), 0, 1))));
	// target port index out of range
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex, Edge(ListMultiIndexSequence(), ListMultiIndexSequence(), 1, 0))));
	// port types don't match
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex_1, Edge(ListMultiIndexSequence(), ListMultiIndexSequence(), 0, 0))));
	default_vertex_1.output_port.set_type(Dummy0VertexPortType());
	// execution instance transition constraint combination
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex_1, Edge(ListMultiIndexSequence(), ListMultiIndexSequence()))));
	default_vertex_1.output_port.execution_instance_transition_constraint =
	    Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported;
	// channels not within input port
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex_1, Edge(
	                          ListMultiIndexSequence({MultiIndex({3})}),
	                          ListMultiIndexSequence({MultiIndex({3})})))));

	EXPECT_TRUE((default_vertex.valid_edge_from(
	    default_vertex_1, Edge(
	                          ListMultiIndexSequence({MultiIndex({1})}),
	                          ListMultiIndexSequence({MultiIndex({1})})))));
}

CEREAL_REGISTER_TYPE(grenade_common_vertex_tests::Dummy0VertexPortType)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::Vertex::Port::Type, grenade_common_vertex_tests::Dummy0VertexPortType)
CEREAL_REGISTER_TYPE(grenade_common_vertex_tests::Dummy1VertexPortType)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::Vertex::Port::Type, grenade_common_vertex_tests::Dummy1VertexPortType)
CEREAL_REGISTER_TYPE(grenade_common_vertex_tests::DummyDefaultVertex)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::Vertex, grenade_common_vertex_tests::DummyDefaultVertex)

TEST(Vertex, Cerealization)
{
	Dummy0VertexPortType port_type_0;
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	Vertex::Port port_0(
	    port_type_0, Vertex::Port::SumOrSplitSupport::yes,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    Vertex::Port::RequiresOrGeneratesData::yes, channels_0);

	DummyDefaultVertex obj{port_0, port_0};

	DummyDefaultVertex obj2;

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
