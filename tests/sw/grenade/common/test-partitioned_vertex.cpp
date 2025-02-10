#include "grenade/common/partitioned_vertex.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <stdexcept>
#include <cereal/archives/json.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace grenade_common_partitioned_vertex_tests {

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


struct DummyDefaultVertex : public PartitionedVertex
{
	Port input_port;
	Port output_port;

	DummyDefaultVertex() = default;
	DummyDefaultVertex(Port in, Port out, std::optional<ExecutionInstanceOnExecutor> id) :
	    PartitionedVertex(), input_port(in), output_port(out)
	{
		set_execution_instance_on_executor(id);
	}

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
		return PartitionedVertex::is_equal_to(other) &&
		       input_port == static_cast<DummyDefaultVertex const&>(other).input_port &&
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
		ar(cereal::base_class<PartitionedVertex>(this));
		ar(input_port);
		ar(output_port);
	}
};

} // namespace grenade_common_partitioned_vertex_tests

using namespace grenade_common_partitioned_vertex_tests;

TEST(PartitionedVertex, General)
{
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	Dummy0VertexPortType port_type_0;

	Vertex::Port port_0(
	    port_type_0, Vertex::Port::SumOrSplitSupport::yes,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    Vertex::Port::RequiresOrGeneratesData::no, channels_0);

	DummyDefaultVertex default_vertex{
	    port_0, port_0,
	    ExecutionInstanceOnExecutor(ExecutionInstanceID(1), ConnectionOnExecutor())};

	auto strong_component_invariant = default_vertex.get_strong_component_invariant();
	assert(strong_component_invariant);
	EXPECT_EQ(*strong_component_invariant, *strong_component_invariant);
	EXPECT_EQ(
	    default_vertex.get_execution_instance_on_executor(),
	    ExecutionInstanceOnExecutor(ExecutionInstanceID(1), ConnectionOnExecutor()));
	dynamic_cast<PartitionedVertex::StrongComponentInvariant&>(*strong_component_invariant)
	    .execution_instance_on_executor = ExecutionInstanceOnExecutor();
	auto strong_component_invariant_1 = strong_component_invariant->copy();
	dynamic_cast<PartitionedVertex::StrongComponentInvariant&>(*strong_component_invariant_1)
	    .execution_instance_on_executor =
	    ExecutionInstanceOnExecutor(ExecutionInstanceID(2), ConnectionOnExecutor());
	EXPECT_NE(*strong_component_invariant, *strong_component_invariant_1);
	dynamic_cast<PartitionedVertex::StrongComponentInvariant&>(*strong_component_invariant_1)
	    .execution_instance_on_executor = ExecutionInstanceOnExecutor();
	EXPECT_EQ(*strong_component_invariant, *strong_component_invariant_1);

	DummyDefaultVertex default_vertex_1{
	    port_0, port_0,
	    ExecutionInstanceOnExecutor(ExecutionInstanceID(2), ConnectionOnExecutor())};

	// vertex not valid (target port index out of range)
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex, Edge(
	                        ListMultiIndexSequence({MultiIndex({1})}),
	                        ListMultiIndexSequence({MultiIndex({1})}), 0, 1))));
	// execution instance transition support combination (no transition when expected)
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex, Edge(
	                        ListMultiIndexSequence({MultiIndex({1})}),
	                        ListMultiIndexSequence({MultiIndex({1})})))));
	// execution instance transition support combination (transition when not expected)
	default_vertex.input_port.execution_instance_transition_constraint =
	    Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported;
	default_vertex.output_port.execution_instance_transition_constraint =
	    Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported;
	EXPECT_FALSE((default_vertex.valid_edge_from(
	    default_vertex_1, Edge(
	                          ListMultiIndexSequence({MultiIndex({1})}),
	                          ListMultiIndexSequence({MultiIndex({1})})))));

	EXPECT_TRUE((default_vertex.valid_edge_from(
	    default_vertex, Edge(
	                        ListMultiIndexSequence({MultiIndex({1})}),
	                        ListMultiIndexSequence({MultiIndex({1})})))));

	default_vertex.input_port.execution_instance_transition_constraint =
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required;
	default_vertex.output_port.execution_instance_transition_constraint =
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required;

	// vertex not valid (source port index out of range)
	EXPECT_FALSE((default_vertex.valid_edge_to(
	    default_vertex, Edge(
	                        ListMultiIndexSequence({MultiIndex({1})}),
	                        ListMultiIndexSequence({MultiIndex({1})}), 1, 0))));
	// execution instance transition support combination (no transition when expected)
	EXPECT_FALSE((default_vertex.valid_edge_to(
	    default_vertex, Edge(
	                        ListMultiIndexSequence({MultiIndex({1})}),
	                        ListMultiIndexSequence({MultiIndex({1})})))));
	// execution instance transition support combination (transition when not expected)
	default_vertex.input_port.execution_instance_transition_constraint =
	    Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported;
	default_vertex.output_port.execution_instance_transition_constraint =
	    Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported;
	EXPECT_FALSE((default_vertex.valid_edge_to(
	    default_vertex_1, Edge(
	                          ListMultiIndexSequence({MultiIndex({1})}),
	                          ListMultiIndexSequence({MultiIndex({1})})))));

	EXPECT_TRUE((default_vertex.valid_edge_to(
	    default_vertex, Edge(
	                        ListMultiIndexSequence({MultiIndex({1})}),
	                        ListMultiIndexSequence({MultiIndex({1})})))));
}

CEREAL_REGISTER_TYPE(grenade_common_partitioned_vertex_tests::Dummy0VertexPortType)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::Vertex::Port::Type,
    grenade_common_partitioned_vertex_tests::Dummy0VertexPortType)
CEREAL_REGISTER_TYPE(grenade_common_partitioned_vertex_tests::Dummy1VertexPortType)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::Vertex::Port::Type,
    grenade_common_partitioned_vertex_tests::Dummy1VertexPortType)
CEREAL_REGISTER_TYPE(grenade_common_partitioned_vertex_tests::DummyDefaultVertex)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::Vertex, grenade_common_partitioned_vertex_tests::DummyDefaultVertex)

TEST(PartitionedVertex, Cerealization)
{
	Dummy0VertexPortType port_type_0;
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	Vertex::Port port_0(
	    port_type_0, Vertex::Port::SumOrSplitSupport::yes,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    Vertex::Port::RequiresOrGeneratesData::yes, channels_0);

	DummyDefaultVertex obj{
	    port_0, port_0,
	    ExecutionInstanceOnExecutor(ExecutionInstanceID(12), ConnectionOnExecutor())};

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
