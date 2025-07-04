#pragma once
#include "grenade/common/detail/constructor_transform.h"
#include "grenade/common/detail/graph.h"
#include "grenade/common/edge_on_graph.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/property.h"
#include "grenade/common/property_holder.h"
#include "grenade/common/vertex_on_graph.h"
#include "hate/visibility.h"
#include <memory>
#include <unordered_map>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/isomorphism.hpp>
#include <boost/graph/vf2_sub_graph_iso.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/property_map/property_map.hpp>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Graph containing storage for element properties.
 * @tparam Derived Derived graph class
 * @tparam Backend Backend graph implementation without storage, required to be a
 * boost::adjacency_list with descriptor stability across mutable operations and bidirectional
 * or undirected directionality
 * @tparam VertexT Vertex property type
 * @tparam EdgeT Edge property type
 * @tparam VertexDescriptorT Vertex descriptor type
 * @tparam EdgeDescriptorT Edge descriptor type
 * @tparam Holder Storage template to store EdgeT and VertexT for each element in the graph
 */
template <
    typename Derived,
    typename Backend,
    typename VertexT,
    typename EdgeT,
    typename VertexDescriptorT,
    typename EdgeDescriptorT,
    template <typename...>
    typename Holder>
struct SYMBOL_VISIBLE Graph
{
	typedef VertexT Vertex;
	typedef EdgeT Edge;
	typedef VertexDescriptorT VertexDescriptor;
	typedef EdgeDescriptorT EdgeDescriptor;
	typedef Graph BaseGraph;

	/**
	 * Construct graph without elements.
	 */
	Graph();

	/**
	 * Copy graph.
	 * The copy creates new descriptors for every element.
	 */
	Graph(Graph const& other);

	/**
	 * Move graph.
	 */
	Graph(Graph&& other);

	/**
	 * Copy graph.
	 * The copy creates new descriptors for every element.
	 */
	Graph& operator=(Graph const& other);

	/**
	 * Move graph.
	 */
	Graph& operator=(Graph&& other);

	/**
	 * Add vertex into graph.
	 * @param vertex Vertex to add
	 * @return Descriptor to vertex in graph
	 */
	virtual VertexDescriptor add_vertex(Vertex const& vertex);

	/**
	 * Add vertex into graph.
	 * @param vertex Vertex to add
	 * @return Descriptor to vertex in graph
	 */
	virtual VertexDescriptor add_vertex(Vertex&& vertex) GENPYBIND(hidden);

	/**
	 * Add edge between vertices into graph.
	 * @param source Source vertex descriptor of edge to add
	 * @param target Target vertex descriptor of edge to add
	 * @param edge Edge property
	 * @return Edge descriptor which in the case of an undirected graph matches the descriptors
	 * given by out_edges
	 * @throws std::runtime_error On edge yielding an invalid graph
	 * @throws std::runtime_error On edge already present in graph
	 * @throws std::out_of_range On source or target of edge not present in graph
	 */
	virtual EdgeDescriptor add_edge(
	    VertexDescriptor const& source, VertexDescriptor const& target, Edge const& edge);

	/**
	 * Add edge between vertices into graph.
	 * @param source Source vertex descriptor of edge to add
	 * @param target Target vertex descriptor of edge to add
	 * @param edge Edge property
	 * @return Edge descriptor which in the case of an undirected graph matches the descriptors
	 * given by out_edges
	 * @throws std::runtime_error On edge yielding an invalid graph
	 * @throws std::runtime_error On edge already present in graph
	 * @throws std::out_of_range On source or target of edge not present in graph
	 */
	virtual EdgeDescriptor add_edge(
	    VertexDescriptor const& source, VertexDescriptor const& target, Edge&& edge)
	    GENPYBIND(hidden);

	/**
	 * Removes all edges between source and target vertex from graph.
	 * @param source Vertex descriptor of source of edge(s) to remove
	 * @param target Vertex descriptor of target of edge(s) to remove
	 * @throws std::out_of_range On vertices not present in graph
	 */
	virtual void remove_edge(VertexDescriptor const& source, VertexDescriptor const& target);

	/**
	 * Remove edge from graph.
	 * @param descriptor Edge descriptor to remove
	 * @throws std::out_of_range On edge not present in graph
	 */
	virtual void remove_edge(EdgeDescriptor const& descriptor);

	/**
	 * Remove all in-edges of a vertex.
	 * @param descriptor Vertex descriptor
	 * @throws std::out_of_range On vertex not present in graph
	 */
	virtual void clear_in_edges(VertexDescriptor const& descriptor);

	/**
	 * Remove all out-edges of a vertex.
	 * @param descriptor Vertex descriptor
	 * @throws std::out_of_range On vertex not present in graph
	 */
	virtual void clear_out_edges(VertexDescriptor const& descriptor);

	/**
	 * Remove all edges to and from a vertex.
	 * @param descriptor Vertex descriptor
	 * @throws std::out_of_range On vertex not present in graph
	 */
	virtual void clear_vertex(VertexDescriptor const& descriptor);

	/**
	 * Remove vertex.
	 * @param descriptor Vertex descriptor
	 * @throws std::out_of_range On vertex not present in graph
	 * @throws std::runtime_error On edges being present to or from vertex
	 */
	virtual void remove_vertex(VertexDescriptor const& descriptor);

	/**
	 * Clear graph. Removes all edges and vertices of graph.
	 */
	void clear_graph();

	/**
	 * Get vertex property.
	 * @param descriptor Vertex descriptor
	 * @throws std::out_of_range On vertex not being present in graph
	 */
	Vertex const& get(VertexDescriptor const& descriptor) const GENPYBIND(hidden);

	GENPYBIND_MANUAL({
		typedef typename decltype(parent)::type self_type;
		parent.def(
		    "get",
		    [](GENPYBIND_PARENT_TYPE const& self,
		       typename self_type::VertexDescriptor const& descriptor)
		        -> std::shared_ptr<typename self_type::Vertex const> {
			    return self.get(descriptor).shared_from_this();
		    });
	})

	/**
	 * Set vertex property.
	 * @param descriptor Vertex descriptor
	 * @param vertex Vertex property
	 * @throws std::out_of_range On vertex not being present in graph
	 * @throws std::runtime_error On vertex property change yielding invalid graph
	 */
	void set(VertexDescriptor const& descriptor, Vertex const& vertex);

	/**
	 * Set vertex property.
	 * @param descriptor Vertex descriptor
	 * @param vertex Vertex property
	 * @throws std::out_of_range On vertex not being present in graph
	 * @throws std::runtime_error On vertex property change yielding invalid graph
	 */
	void set(VertexDescriptor const& descriptor, Vertex&& vertex) GENPYBIND(hidden);

	/**
	 * Get edge property.
	 * @param descriptor Edge descriptor
	 * @throws std::out_of_range On edge not being present in graph
	 */
	Edge const& get(EdgeDescriptor const& descriptor) const;

	/**
	 * Set edge property.
	 * @param descriptor Edge descriptor
	 * @param vertex Edge property
	 * @throws std::out_of_range On edge not being present in graph
	 * @throws std::runtime_error On edge property change yielding invalid graph
	 */
	void set(EdgeDescriptor const& descriptor, Edge const& vertex);

	/**
	 * Set edge property.
	 * @param descriptor Edge descriptor
	 * @param vertex Edge property
	 * @throws std::out_of_range On edge not being present in graph
	 * @throws std::runtime_error On edge property change yielding invalid graph
	 */
	void set(EdgeDescriptor const& descriptor, Edge&& vertex) GENPYBIND(hidden);

	typedef boost::transform_iterator<
	    typename detail::ConstructorTransform<VertexDescriptor>,
	    typename Backend::vertex_iterator>
	    VertexIterator;
	typedef boost::transform_iterator<
	    typename detail::ConstructorTransform<VertexDescriptor>,
	    typename Backend::adjacency_iterator>
	    AdjacencyIterator;
	typedef boost::transform_iterator<
	    typename detail::ConstructorTransform<VertexDescriptor>,
	    typename Backend::inv_adjacency_iterator>
	    InvAdjacencyIterator;
	typedef boost::transform_iterator<
	    typename detail::ConstructorTransform<EdgeDescriptor>,
	    typename Backend::edge_iterator>
	    EdgeIterator;
	typedef boost::transform_iterator<
	    typename detail::ConstructorTransform<EdgeDescriptor>,
	    typename Backend::out_edge_iterator>
	    OutEdgeIterator;
	typedef boost::transform_iterator<
	    typename detail::ConstructorTransform<EdgeDescriptor>,
	    typename Backend::in_edge_iterator>
	    InEdgeIterator;

	/**
	 * Get iterator range over vertices.
	 */
	std::pair<VertexIterator, VertexIterator> vertices() const GENPYBIND(hidden);

	GENPYBIND_MANUAL({
		typedef typename decltype(parent)::type self_type;
		parent.def("vertices", [](GENPYBIND_PARENT_TYPE const& self) {
			auto const vertices = self.vertices();
			return std::vector<typename self_type::VertexDescriptor>(
			    vertices.first, vertices.second);
		});
	})

	/**
	 * Get iterator range over edges.
	 */
	std::pair<EdgeIterator, EdgeIterator> edges() const GENPYBIND(hidden);

	/**
	 * Get iterator range over vertices adjacent to given vertex.
	 * This is the case if there's an edge from the given vertex to the other vertex.
	 * @param descriptor Vertex descriptor
	 * @throws std::out_of_range On vertex not present in graph
	 */
	std::pair<AdjacencyIterator, AdjacencyIterator> adjacent_vertices(
	    VertexDescriptor const& descriptor) const GENPYBIND(hidden);

	/**
	 * Get iterator range over vertices to which given vertex is adjacent.
	 * This is the case if there's an edge from the other vertex to the given vertex.
	 * @param descriptor Vertex descriptor
	 * @throws std::out_of_range On vertex not present in graph
	 */
	std::pair<InvAdjacencyIterator, InvAdjacencyIterator> inv_adjacent_vertices(
	    VertexDescriptor const& descriptor) const GENPYBIND(hidden);

	/**
	 * Get iterator range over out-edges of given vertex.
	 * For an undirected graph, the edge descriptors given here match the edge properties.
	 * @param descriptor Vertex descriptor
	 * @throws std::out_of_range On vertex not present in graph
	 */
	std::pair<OutEdgeIterator, OutEdgeIterator> out_edges(VertexDescriptor const& descriptor) const
	    GENPYBIND(hidden);

	/**
	 * Get iterator range over in-edges of given vertex.
	 * For an undirected graph, the edge descriptors given here don't match the edge properties
	 * since they are describing the reversed edges.
	 * @param descriptor Vertex descriptor
	 * @throws std::out_of_range On vertex not present in graph
	 */
	std::pair<InEdgeIterator, InEdgeIterator> in_edges(VertexDescriptor const& descriptor) const
	    GENPYBIND(hidden);

	GENPYBIND_MANUAL({
		typedef typename decltype(parent)::type self_type;
		parent.def(
		    "in_edges", [](GENPYBIND_PARENT_TYPE const& self,
		                   typename self_type::VertexDescriptor const& descriptor) {
			    auto const in_edges = self.in_edges(descriptor);
			    return std::vector<typename self_type::EdgeDescriptor>(
			        in_edges.first, in_edges.second);
		    });
	})

	/**
	 * Get source vertex of given edge.
	 * @param descriptor Edge descriptor
	 * @throws std::out_of_range On edge not present in graph
	 */
	VertexDescriptor source(EdgeDescriptor const& descriptor) const;

	/**
	 * Get target vertex of given edge.
	 * @param descriptor Edge descriptor
	 * @throws std::out_of_range On edge not present in graph
	 */
	VertexDescriptor target(EdgeDescriptor const& descriptor) const;

	/**
	 * Get out-degree of given vertex.
	 * @param descriptor Vertex descriptor
	 * @throws std::out_of_range On vertex not present in graph
	 */
	size_t out_degree(VertexDescriptor const& descriptor) const;

	/**
	 * Get in-degree of given vertex.
	 * @param descriptor Vertex descriptor
	 * @throws std::out_of_range On vertex not present in graph
	 */
	size_t in_degree(VertexDescriptor const& descriptor) const;

	/**
	 * Get number of vertices in graph.
	 */
	size_t num_vertices() const;

	/**
	 * Get number of edges in graph.
	 */
	size_t num_edges() const;

	/**
	 * Get iterator range over out-edges between given source and target vertex.
	 * For an undirected graph, the edge descriptors given here match the edge properties.
	 * @param source Source vertex descriptor
	 * @param target Target vertex descriptor
	 * @throws std::out_of_range On source or target vertex not present in graph
	 */
	std::pair<OutEdgeIterator, OutEdgeIterator> edge_range(
	    VertexDescriptor const& source, VertexDescriptor const& target) const GENPYBIND(hidden);

	/**
	 * Get whether graph contains given vertex.
	 * @param descriptor Descriptor to vertex
	 */
	bool contains(VertexDescriptor const& descriptor) const;

	/**
	 * Get whether graph contains given edge.
	 * @param descriptor Descriptor to edge
	 */
	bool contains(EdgeDescriptor const& descriptor) const;

	/**
	 * Get strong components of graph.
	 * For each vertex, the component ID is returned.
	 */
	std::map<VertexDescriptor, size_t> strong_components() const;

	/**
	 * Checks whether graph is valid.
	 * Defaults to true.
	 */
	virtual bool valid() const;

	/**
	 * Return if graph is fully connected
	 */
	bool is_connected() const;

	/**
	 * Return Mapping of Vertex Descriptors to indices
	 */
	std::map<typename VertexDescriptor::Value, size_t> get_vertex_index_map() const;

	/**
	 * Return one possible mapping of vertices of compared graphs.
	 * If no mapping is possible an empty map is returned.
	 * @param other Graph compared against.
	 */
	std::map<VertexDescriptor, VertexDescriptor> isomorphism(Graph const& other) const;

	/**
	 * Create a mapping of vertices if the graphs are isomorphic and the number of unmapped
	 * vertices if the graphs only have subgraph-isomorphisms.
	 * @param other Graph compared against.
	 * @param callback Callback function that returns whether the search for other subgraph
	 * isomorphisms should be continued. Inserts mapping to result. Takes number of unmapped
	 * compartments, mapping from Graph to other Graph and reversed mapping and returns a bool. If
	 * true another mapping of vertices is searched.
	 * @param vertex_equivalent Function that returns if two vertices are equivalent.
	 */
	template <typename Callback, typename VertexEquivalent>
	void isomorphism(
	    Graph const& other, Callback&& callback, VertexEquivalent&& vertex_equivalent) const;

	/**
	 * Create a mapping of vertices if the graphs are isomorphic and the number of unmapped
	 * vertices if the graphs only have subgraph-isomorphisms.
	 * @param other Graph compared against.
	 * @param callback Callback function that returns whether the search for other subgraph
	 * isomorphisms should be continued. Inserts mapping to result. Takes number of unmapped
	 * compartments, mapping from Graph to other Graph and reversed mapping and returns a bool. If
	 * true another mapping of vertices is searched.
	 * @param vertex_equivalent Function that returns if two vertices are equivalent.
	 */
	template <typename Callback, typename VertexEquivalent>
	void isomorphism_subgraph(
	    Graph const& other, Callback&& callback, VertexEquivalent&& vertex_equivalent) const;

	/**
	 * Get whether graphs are equal except their descriptors.
	 * This is the case exactly if all vertex and edge properties are equal and in the same order as
	 * well as the edges connect the same vertices in the same order.
	 */
	bool equal_except_descriptors(Graph const& other) const;

	/**
	 * Get whether graphs are equal.
	 * This is the case exactly if all vertex descriptors and properties as well as all edge
	 * properties match in order and the edges connect the same vertices.
	 */
	bool operator==(Graph const& other) const;
	bool operator!=(Graph const& other) const;

private:
	Backend& backend() const;
	std::unique_ptr<Backend> m_backend;
	std::unordered_map<VertexDescriptor, PropertyHolder<Vertex, Holder>> m_vertices;
	std::unordered_map<EdgeDescriptor, PropertyHolder<Edge, Holder>> m_edges;

	void check_contains(VertexDescriptor const& descriptor, char const* description) const;
	void check_contains(EdgeDescriptor const& descriptor, char const* description) const;

	void is_connected_rec(
	    VertexDescriptor const& vertex, std::set<VertexDescriptor>& marked_vertices) const;

	static_assert(std::is_base_of_v<VertexOnGraph<VertexDescriptor, Backend>, VertexDescriptor>);
	static_assert(std::is_base_of_v<EdgeOnGraph<EdgeDescriptor, Backend>, EdgeDescriptor>);
	static_assert(detail::IsSupportedGraph<Backend>::value);
};

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
std::ostream& operator<<(
    std::ostream& os,
    Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder> const& value)
    SYMBOL_VISIBLE;

} // namespace common
} // namespace grenade

#include "grenade/common/graph.tcc"