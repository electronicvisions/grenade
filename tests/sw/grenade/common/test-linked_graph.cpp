#include "grenade/common/linked_graph.h"

#include "dapr/property.h"
#include "grenade/common/detail/graph.h"
#include "grenade/common/graph_impl.tcc"
#include "grenade/common/linked_graph_impl.tcc"
#include "halco/common/geometry.h"
#include "halco/common/geometry_numeric_limits.h"
#include <memory>
#include <stdexcept>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace grenade_common_graph_tests {

struct DummyVertex : public dapr::Property<DummyVertex>
{
	int value{};

	DummyVertex() = default;
	DummyVertex(int value) : value(value) {}

	virtual std::unique_ptr<DummyVertex> copy() const override
	{
		return std::make_unique<DummyVertex>(*this);
	}

	virtual std::unique_ptr<DummyVertex> move() override
	{
		return std::make_unique<DummyVertex>(std::move(*this));
	}

protected:
	virtual std::ostream& print(std::ostream& os) const override
	{
		return os << "DummyVertex(" << value << ")";
	}

	virtual bool is_equal_to(DummyVertex const& other) const override
	{
		return value == other.value;
	}
};


struct DummyEdge : public dapr::Property<DummyEdge>
{
	int value{};

	DummyEdge() = default;
	DummyEdge(int value) : value(value) {}

	virtual std::unique_ptr<DummyEdge> copy() const override
	{
		return std::make_unique<DummyEdge>(*this);
	}

	virtual std::unique_ptr<DummyEdge> move() override
	{
		return std::make_unique<DummyEdge>(std::move(*this));
	}

protected:
	virtual std::ostream& print(std::ostream& os) const override
	{
		return os << "DummyEdge(" << value << ")";
	}

	virtual bool is_equal_to(DummyEdge const& other) const override
	{
		return value == other.value;
	}
};


struct DummyInterGraphHyperEdge : public dapr::Property<DummyInterGraphHyperEdge>
{
	int value{};

	DummyInterGraphHyperEdge() = default;
	DummyInterGraphHyperEdge(int value) : value(value) {}

	virtual std::unique_ptr<DummyInterGraphHyperEdge> copy() const override
	{
		return std::make_unique<DummyInterGraphHyperEdge>(*this);
	}

	virtual std::unique_ptr<DummyInterGraphHyperEdge> move() override
	{
		return std::make_unique<DummyInterGraphHyperEdge>(std::move(*this));
	}

protected:
	virtual std::ostream& print(std::ostream& os) const override
	{
		return os << "DummyInterGraphHyperEdge(" << value << ")";
	}

	virtual bool is_equal_to(DummyInterGraphHyperEdge const& other) const override
	{
		return value == other.value;
	}
};


struct DerivedDummyVertex : public DummyVertex
{
	int derived_value;

	DerivedDummyVertex() = default;
	DerivedDummyVertex(int value, int derived_value) :
	    DummyVertex(value), derived_value(derived_value)
	{
	}

	virtual std::unique_ptr<DummyVertex> copy() const override
	{
		return std::make_unique<DerivedDummyVertex>(*this);
	}

	virtual std::unique_ptr<DummyVertex> move() override
	{
		return std::make_unique<DerivedDummyVertex>(std::move(*this));
	}

protected:
	virtual std::ostream& print(std::ostream& os) const override
	{
		return os << "DerivedDummyVertex(" << value << ", " << derived_value << ")";
	}

	virtual bool is_equal_to(DummyVertex const& other) const override
	{
		return derived_value == static_cast<DerivedDummyVertex const&>(other).derived_value &&
		       DummyVertex::is_equal_to(other);
	}
};


struct DerivedDummyEdge : public DummyEdge
{
	int derived_value{};

	DerivedDummyEdge() = default;
	DerivedDummyEdge(int value, int derived_value) : DummyEdge(value), derived_value(derived_value)
	{
	}

	virtual std::unique_ptr<DummyEdge> copy() const override
	{
		return std::make_unique<DerivedDummyEdge>(*this);
	}

	virtual std::unique_ptr<DummyEdge> move() override
	{
		return std::make_unique<DerivedDummyEdge>(std::move(*this));
	}

protected:
	virtual std::ostream& print(std::ostream& os) const override
	{
		return os << "DerivedDummyEdge(" << value << ", " << derived_value << ")";
	}

	virtual bool is_equal_to(DummyEdge const& other) const override
	{
		return derived_value == static_cast<DerivedDummyEdge const&>(other).derived_value &&
		       DummyEdge::is_equal_to(other);
	}
};


struct DerivedDummyInterGraphHyperEdge : public DummyInterGraphHyperEdge
{
	int derived_value;

	DerivedDummyInterGraphHyperEdge() = default;
	DerivedDummyInterGraphHyperEdge(int value, int derived_value) :
	    DummyInterGraphHyperEdge(value), derived_value(derived_value)
	{
	}

	virtual std::unique_ptr<DummyInterGraphHyperEdge> copy() const override
	{
		return std::make_unique<DerivedDummyInterGraphHyperEdge>(*this);
	}

	virtual std::unique_ptr<DummyInterGraphHyperEdge> move() override
	{
		return std::make_unique<DerivedDummyInterGraphHyperEdge>(std::move(*this));
	}

protected:
	virtual std::ostream& print(std::ostream& os) const override
	{
		return os << "DerivedDummyInterGraphHyperEdge(" << value << ", " << derived_value << ")";
	}

	virtual bool is_equal_to(DummyInterGraphHyperEdge const& other) const override
	{
		return derived_value ==
		           static_cast<DerivedDummyInterGraphHyperEdge const&>(other).derived_value &&
		       DummyInterGraphHyperEdge::is_equal_to(other);
	}
};


struct VertexOnDummyGraph : public halco::common::detail::BaseType<VertexOnDummyGraph, size_t>
{
	constexpr VertexOnDummyGraph(value_type const& value = 0) : base_t(value) {}
};


struct EdgeOnDummyGraph : public halco::common::detail::BaseType<EdgeOnDummyGraph, size_t>
{
	constexpr EdgeOnDummyGraph(value_type const& value = 0) : base_t(value) {}
};


struct InterGraphHyperEdgeOnDummyGraph
    : public halco::common::detail::BaseType<InterGraphHyperEdgeOnDummyGraph, size_t>
{
	constexpr InterGraphHyperEdgeOnDummyGraph(value_type const& value = 0) : base_t(value) {}
};

} // namespace grenade_common_graph_tests

using namespace grenade_common_graph_tests;

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade_common_graph_tests::VertexOnDummyGraph)
HALCO_GEOMETRY_HASH_CLASS(grenade_common_graph_tests::EdgeOnDummyGraph)
HALCO_GEOMETRY_HASH_CLASS(grenade_common_graph_tests::InterGraphHyperEdgeOnDummyGraph)

HALCO_GEOMETRY_NUMERIC_LIMITS_CLASS(grenade_common_graph_tests::VertexOnDummyGraph)
HALCO_GEOMETRY_NUMERIC_LIMITS_CLASS(grenade_common_graph_tests::EdgeOnDummyGraph)
HALCO_GEOMETRY_NUMERIC_LIMITS_CLASS(grenade_common_graph_tests::InterGraphHyperEdgeOnDummyGraph)

} // namespace std

struct BidirectionalDummyGraph
    : public Graph<
          BidirectionalDummyGraph,
          detail::BidirectionalGraph,
          DummyVertex,
          DummyEdge,
          VertexOnDummyGraph,
          EdgeOnDummyGraph,
          std::unique_ptr>
{};


struct DummyLinkedGraph
    : public LinkedGraph<
          InterGraphHyperEdgeOnDummyGraph,
          DummyInterGraphHyperEdge,
          BidirectionalDummyGraph,
          BidirectionalDummyGraph>
{
	DummyLinkedGraph(BidirectionalDummyGraph const& reference) : m_reference(reference) {}

	virtual BidirectionalDummyGraph const& get_reference() const override
	{
		return m_reference;
	}

private:
	BidirectionalDummyGraph const& m_reference;
};


TEST(LinkedGraph, General)
{
	BidirectionalDummyGraph reference;
	DummyVertex vertex_1(1);
	auto vertex_on_graph_1 = reference.add_vertex(vertex_1);

	DummyLinkedGraph linked_graph(reference);

	EXPECT_EQ(linked_graph.get_reference(), reference);

	DerivedDummyVertex vertex_2(2, 3);
	auto vertex_on_graph_2 = linked_graph.add_vertex(std::move(vertex_2));
	EXPECT_TRUE(linked_graph.contains(vertex_on_graph_2));
	EXPECT_EQ(linked_graph.num_inter_graph_hyper_edges(), 0);
	EXPECT_FALSE(linked_graph.contains_inter_graph_hyper_edge_to_reference(vertex_on_graph_1));
	EXPECT_FALSE(linked_graph.contains_inter_graph_hyper_edge_to_linked(vertex_on_graph_2));
	EXPECT_FALSE(linked_graph.valid());

	DerivedDummyInterGraphHyperEdge inter_graph_hyper_edge(20, 30);
	auto const inter_graph_hyper_edge_on_linked_graph = linked_graph.add_inter_graph_hyper_edge(
	    {vertex_on_graph_2}, {vertex_on_graph_1}, inter_graph_hyper_edge);
	EXPECT_EQ(linked_graph.num_inter_graph_hyper_edges(), 1);
	EXPECT_TRUE(linked_graph.contains_inter_graph_hyper_edge_to_reference(vertex_on_graph_1));
	EXPECT_TRUE(linked_graph.contains_inter_graph_hyper_edge_to_linked(vertex_on_graph_2));
	EXPECT_TRUE(linked_graph.valid());
	EXPECT_TRUE(linked_graph.contains(inter_graph_hyper_edge_on_linked_graph));
	EXPECT_EQ(linked_graph.get(inter_graph_hyper_edge_on_linked_graph), inter_graph_hyper_edge);
	inter_graph_hyper_edge.derived_value = 40;
	linked_graph.set(inter_graph_hyper_edge_on_linked_graph, inter_graph_hyper_edge);
	EXPECT_EQ(linked_graph.get(inter_graph_hyper_edge_on_linked_graph), inter_graph_hyper_edge);
	EXPECT_EQ(
	    linked_graph.links(inter_graph_hyper_edge_on_linked_graph),
	    InterGraphHyperEdgeVertexDescriptors<VertexOnDummyGraph>{vertex_on_graph_2});
	EXPECT_EQ(
	    linked_graph.references(inter_graph_hyper_edge_on_linked_graph),
	    InterGraphHyperEdgeVertexDescriptors<VertexOnDummyGraph>{vertex_on_graph_1});

	std::unordered_set<InterGraphHyperEdgeOnDummyGraph> inter_graph_hyper_edges_by_linked;
	for (auto const& hyper_edge :
	     linked_graph.inter_graph_hyper_edges_by_linked(vertex_on_graph_2)) {
		inter_graph_hyper_edges_by_linked.insert(hyper_edge);
	}
	EXPECT_EQ(inter_graph_hyper_edges_by_linked.size(), 1);
	EXPECT_TRUE(inter_graph_hyper_edges_by_linked.contains(inter_graph_hyper_edge_on_linked_graph));

	std::unordered_set<InterGraphHyperEdgeOnDummyGraph> inter_graph_hyper_edges_by_reference;
	for (auto const& hyper_edge :
	     linked_graph.inter_graph_hyper_edges_by_reference(vertex_on_graph_1)) {
		inter_graph_hyper_edges_by_reference.insert(hyper_edge);
	}
	EXPECT_EQ(inter_graph_hyper_edges_by_reference.size(), 1);
	EXPECT_TRUE(
	    inter_graph_hyper_edges_by_reference.contains(inter_graph_hyper_edge_on_linked_graph));

	std::unordered_set<InterGraphHyperEdgeOnDummyGraph> inter_graph_hyper_edges;
	for (auto const& hyper_edge : linked_graph.inter_graph_hyper_edges()) {
		inter_graph_hyper_edges.insert(hyper_edge);
	}
	EXPECT_EQ(inter_graph_hyper_edges.size(), 1);
	EXPECT_TRUE(inter_graph_hyper_edges.contains(inter_graph_hyper_edge_on_linked_graph));

	linked_graph.remove_inter_graph_hyper_edge(inter_graph_hyper_edge_on_linked_graph);
	EXPECT_EQ(linked_graph.num_inter_graph_hyper_edges(), 0);

	linked_graph.add_inter_graph_hyper_edge(
	    {vertex_on_graph_2}, {vertex_on_graph_1}, inter_graph_hyper_edge);
	EXPECT_EQ(linked_graph.num_inter_graph_hyper_edges(), 1);
	linked_graph.clear_all_inter_graph_hyper_edges();
	EXPECT_EQ(linked_graph.num_inter_graph_hyper_edges(), 0);
	EXPECT_THROW(
	    linked_graph.remove_inter_graph_hyper_edge(inter_graph_hyper_edge_on_linked_graph),
	    std::out_of_range);
	EXPECT_THROW(
	    linked_graph.references(inter_graph_hyper_edge_on_linked_graph), std::out_of_range);
	EXPECT_THROW(linked_graph.links(inter_graph_hyper_edge_on_linked_graph), std::out_of_range);

	EXPECT_NO_THROW(linked_graph.add_inter_graph_hyper_edge(
	    {vertex_on_graph_2}, {vertex_on_graph_1}, inter_graph_hyper_edge));
	linked_graph.remove_vertex(vertex_on_graph_2);
	EXPECT_THROW(
	    linked_graph.add_inter_graph_hyper_edge(
	        {vertex_on_graph_2}, {vertex_on_graph_1}, inter_graph_hyper_edge),
	    std::out_of_range);
	vertex_on_graph_2 = linked_graph.add_vertex(std::move(vertex_2));

	reference.remove_vertex(vertex_on_graph_1);
	EXPECT_THROW(
	    linked_graph.add_inter_graph_hyper_edge(
	        {vertex_on_graph_2}, {vertex_on_graph_1}, inter_graph_hyper_edge),
	    std::out_of_range);
	vertex_on_graph_1 = reference.add_vertex(vertex_1);

	EXPECT_THROW(
	    linked_graph.set(inter_graph_hyper_edge_on_linked_graph, inter_graph_hyper_edge),
	    std::out_of_range);

	EXPECT_THROW(linked_graph.get(inter_graph_hyper_edge_on_linked_graph), std::out_of_range);

	linked_graph.remove_vertex(vertex_on_graph_2);
	EXPECT_THROW(
	    linked_graph.inter_graph_hyper_edges_by_linked(vertex_on_graph_2), std::out_of_range);
	EXPECT_THROW(
	    linked_graph.contains_inter_graph_hyper_edge_to_linked(vertex_on_graph_2),
	    std::out_of_range);

	reference.remove_vertex(vertex_on_graph_1);
	EXPECT_THROW(
	    linked_graph.inter_graph_hyper_edges_by_reference(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(
	    linked_graph.contains_inter_graph_hyper_edge_to_reference(vertex_on_graph_1),
	    std::out_of_range);
}