#include "grenade/common/graph.h"

#include "grenade/common/detail/graph.h"
#include "grenade/common/edge_on_graph.h"
#include "grenade/common/edge_on_graph_impl.tcc"
#include "grenade/common/graph_impl.tcc"
#include "grenade/common/property.h"
#include "grenade/common/vertex_on_graph.h"
#include "grenade/common/vertex_on_graph_impl.tcc"
#include <memory>
#include <gtest/gtest.h>

using namespace grenade::common;


struct DummyVertex : public Property<DummyVertex>
{
	int value;

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


struct DummyEdge : public Property<DummyEdge>
{
	int value;

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


struct DerivedDummyVertex : public DummyVertex
{
	int derived_value;

	DerivedDummyVertex(int value, int derived_value) :
	    DummyVertex(value), derived_value(derived_value)
	{}

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
	int derived_value;

	DerivedDummyEdge(int value, int derived_value) : DummyEdge(value), derived_value(derived_value)
	{}

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


struct VertexOnBidirectionalDummyGraph
    : public VertexOnGraph<VertexOnBidirectionalDummyGraph, detail::BidirectionalGraph>
{
	VertexOnBidirectionalDummyGraph(Value const& value) : VertexOnGraph(value) {}
};


struct EdgeOnBidirectionalDummyGraph
    : public EdgeOnGraph<EdgeOnBidirectionalDummyGraph, detail::BidirectionalGraph>
{
	EdgeOnBidirectionalDummyGraph(Value const& value) : EdgeOnGraph(value) {}
};


namespace std {

template <>
struct hash<VertexOnBidirectionalDummyGraph>
{
	size_t operator()(VertexOnBidirectionalDummyGraph const& value) const
	{
		return std::hash<typename VertexOnBidirectionalDummyGraph::Base>{}(value);
	}
};

template <>
struct hash<EdgeOnBidirectionalDummyGraph>
{
	size_t operator()(EdgeOnBidirectionalDummyGraph const& value) const
	{
		return std::hash<typename EdgeOnBidirectionalDummyGraph::Base>{}(value);
	}
};

} // namespace std


struct BidirectionalDummyGraph
    : public Graph<
          BidirectionalDummyGraph,
          detail::BidirectionalGraph,
          DummyVertex,
          DummyEdge,
          VertexOnBidirectionalDummyGraph,
          EdgeOnBidirectionalDummyGraph,
          std::unique_ptr>
{};


TEST(BidirectionalGraph, General)
{
	BidirectionalDummyGraph graph;

	EXPECT_EQ(graph.num_vertices(), 0);
	EXPECT_EQ(graph.num_edges(), 0);

	DummyVertex vertex_1(1);
	auto const vertex_on_graph_1 = graph.add_vertex(vertex_1);
	EXPECT_TRUE(graph.contains(vertex_on_graph_1));
	EXPECT_EQ(graph.num_vertices(), 1);
	EXPECT_EQ(graph.num_edges(), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.get(vertex_on_graph_1), vertex_1);

	DummyVertex vertex_1_1(10);
	graph.set(vertex_on_graph_1, vertex_1_1);
	EXPECT_EQ(graph.get(vertex_on_graph_1), vertex_1_1);
	EXPECT_NE(graph.get(vertex_on_graph_1), vertex_1);
	graph.set(vertex_on_graph_1, vertex_1);

	DerivedDummyVertex vertex_2(2, 3);
	auto const vertex_on_graph_2 = graph.add_vertex(std::move(vertex_2));
	EXPECT_TRUE(graph.contains(vertex_on_graph_2));
	EXPECT_EQ(graph.num_vertices(), 2);
	EXPECT_EQ(graph.num_edges(), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_2), 0);
	EXPECT_EQ(graph.get(vertex_on_graph_2), vertex_2);

	DerivedDummyVertex vertex_2_1(20, 30);
	graph.set(vertex_on_graph_2, std::move(vertex_2_1));
	EXPECT_EQ(graph.get(vertex_on_graph_2), vertex_2_1);
	EXPECT_NE(graph.get(vertex_on_graph_2), vertex_2);
	graph.set(vertex_on_graph_2, vertex_2);

	std::unordered_set<VertexOnBidirectionalDummyGraph> vertices;
	for (auto const& vertex : boost::make_iterator_range(graph.vertices())) {
		vertices.insert(vertex);
	}
	EXPECT_EQ(vertices.size(), 2);
	EXPECT_TRUE(vertices.contains(vertex_on_graph_1));
	EXPECT_TRUE(vertices.contains(vertex_on_graph_2));

	DummyEdge edge_1(1);
	auto const edge_on_graph_1 = graph.add_edge(vertex_on_graph_1, vertex_on_graph_2, edge_1);
	EXPECT_TRUE(graph.contains(edge_on_graph_1));
	EXPECT_EQ(graph.num_vertices(), 2);
	EXPECT_EQ(graph.num_edges(), 1);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 1);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 1);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_2), 0);
	EXPECT_EQ(graph.get(edge_on_graph_1), edge_1);
	EXPECT_EQ(graph.source(edge_on_graph_1), vertex_on_graph_1);
	EXPECT_EQ(graph.target(edge_on_graph_1), vertex_on_graph_2);
	EXPECT_THROW(graph.add_edge(vertex_on_graph_1, vertex_on_graph_2, edge_1), std::runtime_error);

	DummyEdge edge_1_1(10);
	graph.set(edge_on_graph_1, edge_1_1);
	EXPECT_EQ(graph.get(edge_on_graph_1), edge_1_1);
	graph.set(edge_on_graph_1, edge_1);

	DerivedDummyEdge edge_2(2, 3);
	auto const edge_on_graph_2 =
	    graph.add_edge(vertex_on_graph_2, vertex_on_graph_1, std::move(edge_2));
	EXPECT_TRUE(graph.contains(edge_on_graph_2));
	EXPECT_EQ(graph.num_vertices(), 2);
	EXPECT_EQ(graph.num_edges(), 2);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_1), 1);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 1);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 1);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_2), 1);
	EXPECT_EQ(graph.get(edge_on_graph_2), edge_2);
	EXPECT_EQ(graph.source(edge_on_graph_2), vertex_on_graph_2);
	EXPECT_EQ(graph.target(edge_on_graph_2), vertex_on_graph_1);
	EXPECT_THROW(graph.add_edge(vertex_on_graph_2, vertex_on_graph_1, edge_2), std::runtime_error);

	DerivedDummyEdge edge_2_2(20, 30);
	graph.set(edge_on_graph_2, std::move(edge_2_2));
	EXPECT_EQ(graph.get(edge_on_graph_2), edge_2_2);
	graph.set(edge_on_graph_2, edge_2);

	std::unordered_set<EdgeOnBidirectionalDummyGraph> edges;
	for (auto const& edge : boost::make_iterator_range(graph.edges())) {
		edges.insert(edge);
	}
	EXPECT_EQ(edges.size(), 2);
	EXPECT_TRUE(edges.contains(edge_on_graph_1));
	EXPECT_TRUE(edges.contains(edge_on_graph_2));

	std::unordered_set<VertexOnBidirectionalDummyGraph> adjacent_vertices;
	for (auto const& vertex :
	     boost::make_iterator_range(graph.adjacent_vertices(vertex_on_graph_1))) {
		adjacent_vertices.insert(vertex);
	}
	EXPECT_EQ(adjacent_vertices.size(), 1);
	EXPECT_TRUE(adjacent_vertices.contains(vertex_on_graph_2));

	std::unordered_set<VertexOnBidirectionalDummyGraph> inv_adjacent_vertices;
	for (auto const& vertex :
	     boost::make_iterator_range(graph.inv_adjacent_vertices(vertex_on_graph_1))) {
		inv_adjacent_vertices.insert(vertex);
	}
	EXPECT_EQ(inv_adjacent_vertices.size(), 1);
	EXPECT_TRUE(inv_adjacent_vertices.contains(vertex_on_graph_2));

	std::unordered_set<EdgeOnBidirectionalDummyGraph> out_edges;
	for (auto const& edge : boost::make_iterator_range(graph.out_edges(vertex_on_graph_1))) {
		out_edges.insert(edge);
	}
	EXPECT_EQ(out_edges.size(), 1);
	EXPECT_TRUE(out_edges.contains(edge_on_graph_1));

	std::unordered_set<EdgeOnBidirectionalDummyGraph> in_edges;
	for (auto const& edge : boost::make_iterator_range(graph.in_edges(vertex_on_graph_1))) {
		in_edges.insert(edge);
	}
	EXPECT_EQ(in_edges.size(), 1);
	EXPECT_TRUE(in_edges.contains(edge_on_graph_2));

	std::unordered_set<EdgeOnBidirectionalDummyGraph> edge_range;
	for (auto const& edge :
	     boost::make_iterator_range(graph.edge_range(vertex_on_graph_1, vertex_on_graph_2))) {
		edge_range.insert(edge);
	}
	EXPECT_EQ(edge_range.size(), 1);
	EXPECT_TRUE(edge_range.contains(edge_on_graph_1));

	std::stringstream ss;
	ss << graph;
	auto const graph_str = ss.str();
	EXPECT_TRUE(graph_str.starts_with("BidirectionalDummyGraph(\n"));
	EXPECT_TRUE(graph_str.ends_with("\n)"));

	Graph graph_copy = graph;

	std::vector<VertexOnBidirectionalDummyGraph> vertices_copy;
	for (auto const& vertex : boost::make_iterator_range(graph_copy.vertices())) {
		vertices_copy.push_back(vertex);
	}
	EXPECT_EQ(vertices_copy.size(), 2);

	std::vector<EdgeOnBidirectionalDummyGraph> edges_copy;
	for (auto const& vertex : boost::make_iterator_range(graph_copy.edges())) {
		edges_copy.push_back(vertex);
	}
	EXPECT_EQ(edges_copy.size(), 2);

	graph.remove_edge(vertex_on_graph_1, vertex_on_graph_2);
	EXPECT_FALSE(graph.contains(edge_on_graph_1));
	EXPECT_EQ(graph_copy.num_edges(), 2);
	EXPECT_THROW(graph.get(edge_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.set(edge_on_graph_1, edge_1), std::out_of_range);
	{
		auto edge_1_copy(edge_1);
		EXPECT_THROW(graph.set(edge_on_graph_1, edge_1), std::out_of_range);
		EXPECT_THROW(graph.set(edge_on_graph_1, std::move(edge_1_copy)), std::out_of_range);
	}
	EXPECT_THROW(graph.source(edge_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.target(edge_on_graph_1), std::out_of_range);
	EXPECT_EQ(graph.num_edges(), 1);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 0);
	auto const edge_on_graph_1_1 = graph.add_edge(vertex_on_graph_1, vertex_on_graph_2, edge_1);
	EXPECT_EQ(graph.num_edges(), 2);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 1);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 1);
	EXPECT_TRUE(graph.contains(edge_on_graph_1_1));

	graph.remove_edge(edge_on_graph_1_1);
	EXPECT_FALSE(graph.contains(edge_on_graph_1_1));
	EXPECT_THROW(graph.get(edge_on_graph_1_1), std::out_of_range);
	EXPECT_THROW(graph.set(edge_on_graph_1_1, edge_1), std::out_of_range);
	{
		auto edge_1_copy(edge_1);
		EXPECT_THROW(graph.set(edge_on_graph_1_1, edge_1), std::out_of_range);
		EXPECT_THROW(graph.set(edge_on_graph_1_1, std::move(edge_1_copy)), std::out_of_range);
	}
	EXPECT_THROW(graph.source(edge_on_graph_1_1), std::out_of_range);
	EXPECT_THROW(graph.target(edge_on_graph_1_1), std::out_of_range);
	EXPECT_EQ(graph.num_edges(), 1);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 0);
	EXPECT_THROW(graph.remove_edge(edge_on_graph_1_1), std::out_of_range);
	graph.add_edge(vertex_on_graph_1, vertex_on_graph_2, edge_1);

	graph.clear_out_edges(vertex_on_graph_1);
	EXPECT_FALSE(graph.contains(edge_on_graph_1));
	EXPECT_EQ(graph.num_edges(), 1);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 0);
	auto const edge_on_graph_1_2 = graph.add_edge(vertex_on_graph_1, vertex_on_graph_2, edge_1);

	graph.clear_in_edges(vertex_on_graph_1);
	EXPECT_FALSE(graph.contains(edge_on_graph_2));
	EXPECT_EQ(graph.num_edges(), 1);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_2), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_1), 0);
	auto const edge_on_graph_2_1 = graph.add_edge(vertex_on_graph_2, vertex_on_graph_1, edge_2);

	EXPECT_THROW(graph.remove_vertex(vertex_on_graph_1), std::runtime_error);

	graph.clear_vertex(vertex_on_graph_1);
	EXPECT_FALSE(graph.contains(edge_on_graph_1_2));
	EXPECT_FALSE(graph.contains(edge_on_graph_2_1));
	EXPECT_EQ(graph.num_edges(), 0);

	graph.remove_vertex(vertex_on_graph_1);
	EXPECT_FALSE(graph.contains(vertex_on_graph_1));
	EXPECT_THROW(graph.get(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.set(vertex_on_graph_1, vertex_1), std::out_of_range);
	EXPECT_THROW(graph.set(vertex_on_graph_1, std::move(vertex_1)), std::out_of_range);
	EXPECT_THROW(graph.in_degree(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.out_degree(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.remove_vertex(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.clear_out_edges(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.clear_in_edges(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.clear_vertex(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.adjacent_vertices(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.inv_adjacent_vertices(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.edge_range(vertex_on_graph_1, vertex_on_graph_2), std::out_of_range);
	EXPECT_THROW(graph.edge_range(vertex_on_graph_2, vertex_on_graph_1), std::out_of_range);
}


struct VertexOnUndirectedDummyGraph
    : public VertexOnGraph<VertexOnUndirectedDummyGraph, detail::UndirectedGraph>
{
	VertexOnUndirectedDummyGraph(Value const& value) : VertexOnGraph(value) {}
};


struct EdgeOnUndirectedDummyGraph
    : public EdgeOnGraph<EdgeOnUndirectedDummyGraph, detail::UndirectedGraph>
{
	EdgeOnUndirectedDummyGraph(Value const& value) : EdgeOnGraph(value) {}
};


namespace std {

template <>
struct hash<VertexOnUndirectedDummyGraph>
{
	size_t operator()(VertexOnUndirectedDummyGraph const& value) const
	{
		return std::hash<typename VertexOnUndirectedDummyGraph::Base>{}(value);
	}
};

template <>
struct hash<EdgeOnUndirectedDummyGraph>
{
	size_t operator()(EdgeOnUndirectedDummyGraph const& value) const
	{
		return std::hash<typename EdgeOnUndirectedDummyGraph::Base>{}(value);
	}
};

} // namespace std


struct UndirectedDummyGraph
    : public Graph<
          UndirectedDummyGraph,
          detail::UndirectedGraph,
          DummyVertex,
          DummyEdge,
          VertexOnUndirectedDummyGraph,
          EdgeOnUndirectedDummyGraph,
          std::unique_ptr>
{};


TEST(UndirectedGraph, General)
{
	UndirectedDummyGraph graph;

	EXPECT_EQ(graph.num_vertices(), 0);
	EXPECT_EQ(graph.num_edges(), 0);

	DummyVertex vertex_1(1);
	auto const vertex_on_graph_1 = graph.add_vertex(vertex_1);
	EXPECT_TRUE(graph.contains(vertex_on_graph_1));
	EXPECT_EQ(graph.num_vertices(), 1);
	EXPECT_EQ(graph.num_edges(), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.get(vertex_on_graph_1), vertex_1);

	DummyVertex vertex_1_1(10);
	graph.set(vertex_on_graph_1, vertex_1_1);
	EXPECT_EQ(graph.get(vertex_on_graph_1), vertex_1_1);
	EXPECT_NE(graph.get(vertex_on_graph_1), vertex_1);
	graph.set(vertex_on_graph_1, vertex_1);

	DerivedDummyVertex vertex_2(2, 3);
	auto const vertex_on_graph_2 = graph.add_vertex(std::move(vertex_2));
	EXPECT_TRUE(graph.contains(vertex_on_graph_2));
	EXPECT_EQ(graph.num_vertices(), 2);
	EXPECT_EQ(graph.num_edges(), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_2), 0);
	EXPECT_EQ(graph.get(vertex_on_graph_2), vertex_2);

	DerivedDummyVertex vertex_2_1(20, 30);
	graph.set(vertex_on_graph_2, std::move(vertex_2_1));
	EXPECT_EQ(graph.get(vertex_on_graph_2), vertex_2_1);
	EXPECT_NE(graph.get(vertex_on_graph_2), vertex_2);
	graph.set(vertex_on_graph_2, vertex_2);

	std::unordered_set<VertexOnUndirectedDummyGraph> vertices;
	for (auto const& vertex : boost::make_iterator_range(graph.vertices())) {
		vertices.insert(vertex);
	}
	EXPECT_EQ(vertices.size(), 2);
	EXPECT_TRUE(vertices.contains(vertex_on_graph_1));
	EXPECT_TRUE(vertices.contains(vertex_on_graph_2));

	DummyEdge edge_1(1);
	auto const edge_on_graph_1 = graph.add_edge(vertex_on_graph_1, vertex_on_graph_2, edge_1);
	EXPECT_TRUE(graph.contains(edge_on_graph_1));
	EXPECT_EQ(graph.num_vertices(), 2);
	EXPECT_EQ(graph.num_edges(), 1);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_1), 1);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 1);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 1);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_2), 1);
	EXPECT_EQ(graph.get(edge_on_graph_1), edge_1);
	EXPECT_EQ(graph.source(edge_on_graph_1), vertex_on_graph_1);
	EXPECT_EQ(graph.target(edge_on_graph_1), vertex_on_graph_2);
	EXPECT_THROW(graph.add_edge(vertex_on_graph_1, vertex_on_graph_2, edge_1), std::runtime_error);
	EXPECT_THROW(graph.add_edge(vertex_on_graph_2, vertex_on_graph_1, edge_1), std::runtime_error);

	DummyEdge edge_1_1(10);
	graph.set(edge_on_graph_1, edge_1_1);
	EXPECT_EQ(graph.get(edge_on_graph_1), edge_1_1);
	graph.set(edge_on_graph_1, edge_1);

	DerivedDummyEdge edge_1_2(20, 30);
	graph.set(edge_on_graph_1, std::move(edge_1_2));
	EXPECT_EQ(graph.get(edge_on_graph_1), edge_1_2);
	graph.set(edge_on_graph_1, edge_1);

	std::unordered_set<EdgeOnUndirectedDummyGraph> edges;
	for (auto const& edge : boost::make_iterator_range(graph.edges())) {
		edges.insert(edge);
	}
	EXPECT_EQ(edges.size(), 1);
	EXPECT_TRUE(edges.contains(edge_on_graph_1));

	std::unordered_set<VertexOnUndirectedDummyGraph> adjacent_vertices;
	for (auto const& vertex :
	     boost::make_iterator_range(graph.adjacent_vertices(vertex_on_graph_1))) {
		adjacent_vertices.insert(vertex);
	}
	EXPECT_EQ(adjacent_vertices.size(), 1);
	EXPECT_TRUE(adjacent_vertices.contains(vertex_on_graph_2));

	std::unordered_set<VertexOnUndirectedDummyGraph> inv_adjacent_vertices;
	for (auto const& vertex :
	     boost::make_iterator_range(graph.inv_adjacent_vertices(vertex_on_graph_1))) {
		inv_adjacent_vertices.insert(vertex);
	}
	EXPECT_EQ(inv_adjacent_vertices.size(), 1);
	EXPECT_TRUE(inv_adjacent_vertices.contains(vertex_on_graph_2));

	std::unordered_set<EdgeOnUndirectedDummyGraph> out_edges;
	for (auto const& edge : boost::make_iterator_range(graph.out_edges(vertex_on_graph_1))) {
		out_edges.insert(edge);
	}
	EXPECT_EQ(out_edges.size(), 1);
	EXPECT_TRUE(out_edges.contains(edge_on_graph_1));

	std::unordered_set<EdgeOnUndirectedDummyGraph> in_edges;
	for (auto const& edge : boost::make_iterator_range(graph.in_edges(vertex_on_graph_1))) {
		in_edges.insert(edge);
	}
	EXPECT_EQ(in_edges.size(), 1);
	EXPECT_TRUE(in_edges.contains(edge_on_graph_1));

	std::unordered_set<EdgeOnUndirectedDummyGraph> edge_range;
	for (auto const& edge :
	     boost::make_iterator_range(graph.edge_range(vertex_on_graph_1, vertex_on_graph_2))) {
		edge_range.insert(edge);
	}
	EXPECT_EQ(edge_range.size(), 1);
	EXPECT_TRUE(edge_range.contains(edge_on_graph_1));

	std::stringstream ss;
	ss << graph;
	auto const graph_str = ss.str();
	EXPECT_TRUE(graph_str.starts_with("UndirectedDummyGraph(\n"));
	EXPECT_TRUE(graph_str.ends_with("\n)"));

	Graph graph_copy = graph;

	std::vector<VertexOnUndirectedDummyGraph> vertices_copy;
	for (auto const& vertex : boost::make_iterator_range(graph_copy.vertices())) {
		vertices_copy.push_back(vertex);
	}
	EXPECT_EQ(vertices_copy.size(), 2);

	std::vector<EdgeOnUndirectedDummyGraph> edges_copy;
	for (auto const& vertex : boost::make_iterator_range(graph_copy.edges())) {
		edges_copy.push_back(vertex);
	}
	EXPECT_EQ(edges_copy.size(), 1);

	graph.remove_edge(vertex_on_graph_1, vertex_on_graph_2);
	EXPECT_FALSE(graph.contains(edge_on_graph_1));
	EXPECT_EQ(graph_copy.num_edges(), 1);
	EXPECT_THROW(graph.get(edge_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.set(edge_on_graph_1, edge_1), std::out_of_range);
	{
		auto edge_1_copy(edge_1);
		EXPECT_THROW(graph.set(edge_on_graph_1, edge_1), std::out_of_range);
		EXPECT_THROW(graph.set(edge_on_graph_1, std::move(edge_1_copy)), std::out_of_range);
	}
	EXPECT_THROW(graph.source(edge_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.target(edge_on_graph_1), std::out_of_range);
	EXPECT_EQ(graph.num_edges(), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 0);
	auto const edge_on_graph_1_1 = graph.add_edge(vertex_on_graph_1, vertex_on_graph_2, edge_1);
	EXPECT_EQ(graph.num_edges(), 1);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_1), 1);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 1);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 1);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_2), 1);
	EXPECT_TRUE(graph.contains(edge_on_graph_1_1));

	graph.remove_edge(edge_on_graph_1_1);
	EXPECT_FALSE(graph.contains(edge_on_graph_1_1));
	EXPECT_THROW(graph.get(edge_on_graph_1_1), std::out_of_range);
	EXPECT_THROW(graph.set(edge_on_graph_1_1, edge_1), std::out_of_range);
	{
		auto edge_1_copy(edge_1);
		EXPECT_THROW(graph.set(edge_on_graph_1_1, edge_1), std::out_of_range);
		EXPECT_THROW(graph.set(edge_on_graph_1_1, std::move(edge_1_copy)), std::out_of_range);
	}
	EXPECT_THROW(graph.source(edge_on_graph_1_1), std::out_of_range);
	EXPECT_THROW(graph.target(edge_on_graph_1_1), std::out_of_range);
	EXPECT_EQ(graph.num_edges(), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_2), 0);
	EXPECT_THROW(graph.remove_edge(edge_on_graph_1_1), std::out_of_range);
	graph.add_edge(vertex_on_graph_1, vertex_on_graph_2, edge_1);

	graph.clear_out_edges(vertex_on_graph_1);
	EXPECT_FALSE(graph.contains(edge_on_graph_1));
	EXPECT_EQ(graph.num_edges(), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_2), 0);
	auto const edge_on_graph_1_2 = graph.add_edge(vertex_on_graph_1, vertex_on_graph_2, edge_1);

	graph.clear_in_edges(vertex_on_graph_1);
	EXPECT_FALSE(graph.contains(edge_on_graph_1_2));
	EXPECT_EQ(graph.num_edges(), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_1), 0);
	EXPECT_EQ(graph.in_degree(vertex_on_graph_2), 0);
	EXPECT_EQ(graph.out_degree(vertex_on_graph_2), 0);
	auto const edge_on_graph_2_1 = graph.add_edge(vertex_on_graph_2, vertex_on_graph_1, edge_1);

	EXPECT_THROW(graph.remove_vertex(vertex_on_graph_1), std::runtime_error);

	graph.clear_vertex(vertex_on_graph_1);
	EXPECT_FALSE(graph.contains(edge_on_graph_2_1));
	EXPECT_EQ(graph.num_edges(), 0);

	graph.remove_vertex(vertex_on_graph_1);
	EXPECT_FALSE(graph.contains(vertex_on_graph_1));
	EXPECT_THROW(graph.get(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.set(vertex_on_graph_1, vertex_1), std::out_of_range);
	EXPECT_THROW(graph.set(vertex_on_graph_1, std::move(vertex_1)), std::out_of_range);
	EXPECT_THROW(graph.in_degree(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.out_degree(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.remove_vertex(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.clear_out_edges(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.clear_in_edges(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.clear_vertex(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.adjacent_vertices(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.inv_adjacent_vertices(vertex_on_graph_1), std::out_of_range);
	EXPECT_THROW(graph.edge_range(vertex_on_graph_1, vertex_on_graph_2), std::out_of_range);
	EXPECT_THROW(graph.edge_range(vertex_on_graph_2, vertex_on_graph_1), std::out_of_range);
}
