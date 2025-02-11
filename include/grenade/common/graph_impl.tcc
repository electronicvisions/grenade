#pragma once
#include "grenade/common/graph.h"

#include "hate/indent.h"
#include "hate/type_index.h"
#include <ostream>
#include <set>
#include <stdexcept>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace grenade::common {

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::Graph(
    Graph const& other) :
    m_graph(), m_vertices(), m_edges()
{
	operator=(other);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::Graph(
    Graph&& other) :
    m_graph(std::move(other.m_graph)),
    m_vertices(std::move(other.m_vertices)),
    m_edges(std::move(other.m_edges))
{}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>&
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::operator=(
    Graph const& other)
{
	if (this != &other) {
		std::unordered_map<VertexDescriptor, VertexDescriptor> vertex_descriptor_translation;
		for (auto const other_descriptor : boost::make_iterator_range(other.vertices())) {
			auto const descriptor = Graph::add_vertex(other.get(other_descriptor));
			vertex_descriptor_translation.emplace(other_descriptor, descriptor);
		}
		for (auto const other_descriptor : boost::make_iterator_range(other.edges())) {
			Graph::add_edge(
			    vertex_descriptor_translation.at(other.source(other_descriptor)),
			    vertex_descriptor_translation.at(other.target(other_descriptor)),
			    other.get(other_descriptor));
		}
	}
	return *this;
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>&
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::operator=(
    Graph&& other)
{
	if (this != &other) {
		m_graph = std::move(other.m_graph);
		m_vertices = std::move(other.m_vertices);
		m_edges = std::move(other.m_edges);
	}
	return *this;
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
VertexDescriptor
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::add_vertex(
    Vertex const& vertex)
{
	VertexDescriptor const descriptor(boost::add_vertex(m_graph));
	m_vertices.emplace(descriptor, vertex);
	return descriptor;
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
VertexDescriptor
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::add_vertex(
    Vertex&& vertex)
{
	VertexDescriptor const descriptor(boost::add_vertex(m_graph));
	m_vertices.emplace(descriptor, std::move(vertex));
	return descriptor;
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
EdgeDescriptor
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::add_edge(
    VertexDescriptor const& source, VertexDescriptor const& target, Edge const& edge)
{
	check_contains(source, "add edge from source");
	check_contains(target, "add edge to target");
	auto const [descriptor_graph, success] =
	    boost::add_edge(source.value(), target.value(), m_graph);
	EdgeDescriptor const descriptor(descriptor_graph);
	if (!success) {
		std::stringstream ss;
		ss << "Trying to add edge from source (" << source << ") to target (" << target
		   << " not successful, since edge is already present (" << descriptor
		   << ") and graph doesn't support multiple edges between the same vertices.";
		throw std::runtime_error(ss.str());
	}
	m_edges.emplace(descriptor, edge);
	return descriptor;
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
EdgeDescriptor
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::add_edge(
    VertexDescriptor const& source, VertexDescriptor const& target, Edge&& edge)
{
	check_contains(source, "add edge from source");
	check_contains(target, "add edge to target");
	auto const [descriptor_graph, success] =
	    boost::add_edge(source.value(), target.value(), m_graph);
	EdgeDescriptor const descriptor(descriptor_graph);
	if (!success) {
		std::stringstream ss;
		ss << "Trying to add edge from source (" << source << ") to target (" << target
		   << " not successful, since edge is already present (" << descriptor
		   << ") and graph doesn't support multiple edges between the same vertices.";
		throw std::runtime_error(ss.str());
	}
	m_edges.emplace(descriptor, std::move(edge));
	return descriptor;
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::remove_edge(
    VertexDescriptor const& source, VertexDescriptor const& target)
{
	for (auto const descriptor : boost::make_iterator_range(edge_range(source, target))) {
		auto const num_elements_removed = m_edges.erase(descriptor);
		assert(num_elements_removed == 1);
	}
	boost::remove_edge(source.value(), target.value(), m_graph);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::remove_edge(
    EdgeDescriptor const& descriptor)
{
	check_contains(descriptor, "remove");
	m_edges.erase(descriptor);
	boost::remove_edge(descriptor.value(), m_graph);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
    clear_in_edges(VertexDescriptor const& descriptor)
{
	std::vector<EdgeDescriptor> edge_descriptors;
	edge_descriptors.reserve(in_degree(descriptor));
	for (auto const edge_descriptor : boost::make_iterator_range(in_edges(descriptor))) {
		edge_descriptors.push_back(edge_descriptor);
	}
	for (auto const edge_descriptor : edge_descriptors) {
		remove_edge(edge_descriptor);
	}
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
    clear_out_edges(VertexDescriptor const& descriptor)
{
	std::vector<EdgeDescriptor> edge_descriptors;
	edge_descriptors.reserve(out_degree(descriptor));
	for (auto const edge_descriptor : boost::make_iterator_range(out_edges(descriptor))) {
		edge_descriptors.push_back(edge_descriptor);
	}
	for (auto const edge_descriptor : edge_descriptors) {
		remove_edge(edge_descriptor);
	}
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::clear_vertex(
    VertexDescriptor const& descriptor)
{
	clear_in_edges(descriptor);
	clear_out_edges(descriptor);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::remove_vertex(
    VertexDescriptor const& descriptor)
{
	check_contains(descriptor, "remove");
	if (in_degree(descriptor) != 0 || out_degree(descriptor) != 0) {
		throw std::runtime_error("Trying to remove vertex which has connected edges.");
	}
	m_vertices.erase(descriptor);
	boost::remove_vertex(descriptor.value(), m_graph);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
Vertex const& Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::get(
    VertexDescriptor const& descriptor) const
{
	check_contains(descriptor, "get");
	return *m_vertices.at(descriptor);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::set(
    VertexDescriptor const& descriptor, Vertex const& vertex)
{
	check_contains(descriptor, "set");
	m_vertices.at(descriptor) = vertex;
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::set(
    VertexDescriptor const& descriptor, Vertex&& vertex)
{
	check_contains(descriptor, "set");
	m_vertices.at(descriptor) = std::move(vertex);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
Edge const& Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::get(
    EdgeDescriptor const& descriptor) const
{
	check_contains(descriptor, "get");
	return *m_edges.at(descriptor);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::set(
    EdgeDescriptor const& descriptor, Edge const& edge)
{
	check_contains(descriptor, "set");
	m_edges.at(descriptor) = edge;
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::set(
    EdgeDescriptor const& descriptor, Edge&& edge)
{
	check_contains(descriptor, "set");
	m_edges.at(descriptor) = std::move(edge);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
std::pair<
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        VertexIterator,
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        VertexIterator>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::vertices() const
{
	return std::pair<VertexIterator, VertexIterator>{boost::vertices(m_graph)};
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
std::pair<
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        EdgeIterator,
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        EdgeIterator>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::edges() const
{
	return std::pair<EdgeIterator, EdgeIterator>{boost::edges(m_graph)};
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
std::pair<
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        AdjacencyIterator,
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        AdjacencyIterator>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::adjacent_vertices(
    VertexDescriptor const& descriptor) const
{
	check_contains(descriptor, "get adjacent vertices of");
	return std::pair<AdjacencyIterator, AdjacencyIterator>{
	    boost::adjacent_vertices(descriptor.value(), m_graph)};
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
std::pair<
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        InvAdjacencyIterator,
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        InvAdjacencyIterator>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
    inv_adjacent_vertices(VertexDescriptor const& descriptor) const
{
	check_contains(descriptor, "get inverse adjacent vertices of");
	return std::pair<InvAdjacencyIterator, InvAdjacencyIterator>{
	    boost::inv_adjacent_vertices(descriptor.value(), m_graph)};
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
std::pair<
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        OutEdgeIterator,
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        OutEdgeIterator>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::out_edges(
    VertexDescriptor const& descriptor) const
{
	check_contains(descriptor, "get out-edges of");
	return std::pair<OutEdgeIterator, OutEdgeIterator>{
	    boost::out_edges(descriptor.value(), m_graph)};
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
std::pair<
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        InEdgeIterator,
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        InEdgeIterator>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::in_edges(
    VertexDescriptor const& descriptor) const
{
	check_contains(descriptor, "get in-edges of");
	return std::pair<InEdgeIterator, InEdgeIterator>{boost::in_edges(descriptor.value(), m_graph)};
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
VertexDescriptor
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::source(
    EdgeDescriptor const& descriptor) const
{
	check_contains(descriptor, "get source of");
	return boost::source(descriptor.value(), m_graph);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
VertexDescriptor
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::target(
    EdgeDescriptor const& descriptor) const
{
	check_contains(descriptor, "get target of");
	return boost::target(descriptor.value(), m_graph);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
size_t Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::out_degree(
    VertexDescriptor const& descriptor) const
{
	check_contains(descriptor, "get out-degree of");
	return boost::out_degree(descriptor.value(), m_graph);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
size_t Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::in_degree(
    VertexDescriptor const& descriptor) const
{
	check_contains(descriptor, "get in-degree of");
	return boost::in_degree(descriptor.value(), m_graph);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
size_t
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::num_vertices()
    const
{
	return boost::num_vertices(m_graph);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
size_t Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::num_edges()
    const
{
	return boost::num_edges(m_graph);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
std::pair<
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        OutEdgeIterator,
    typename Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
        OutEdgeIterator>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::edge_range(
    VertexDescriptor const& source, VertexDescriptor const& target) const
{
	check_contains(source, "get edge_range from source");
	check_contains(target, "get edge_range to target");
	return std::pair<OutEdgeIterator, OutEdgeIterator>{
	    boost::edge_range(source.value(), target.value(), m_graph)};
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
bool Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::contains(
    VertexDescriptor const& descriptor) const
{
	return m_vertices.contains(descriptor);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
bool Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::contains(
    EdgeDescriptor const& descriptor) const
{
	return m_edges.contains(descriptor);
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
std::map<VertexDescriptor, size_t>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::strong_components()
    const
{
	if constexpr (Backend::directed_selector::is_directed) {
		std::map<typename VertexDescriptor::Value, size_t> strongly_connected_component_coloring;
		std::map<typename VertexDescriptor::Value, size_t> vertex_index_map;
		for (size_t vertex_index = 0;
		     auto const vertex_descriptor : boost::make_iterator_range(vertices())) {
			vertex_index_map.emplace(vertex_descriptor.value(), vertex_index);
			vertex_index++;
		}
		boost::strong_components(
		    m_graph, boost::make_assoc_property_map(strongly_connected_component_coloring),
		    boost::vertex_index_map(boost::make_assoc_property_map(vertex_index_map)));
		std::map<VertexDescriptor, size_t> ret;
		for (auto const& [key, value] : strongly_connected_component_coloring) {
			ret.emplace(VertexDescriptor(key), value);
		}
		return ret;
	} else {
		throw std::runtime_error("Strong components not accessible on undirected graph.");
	}
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
bool Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::valid() const
{
	return true;
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
bool Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::operator==(
    Graph const& other) const
{
	return std::equal(
	           boost::vertices(m_graph).first, boost::vertices(m_graph).second,
	           boost::vertices(other.m_graph).first, boost::vertices(other.m_graph).second) &&
	       std::equal(
	           boost::edges(m_graph).first, boost::edges(m_graph).second,
	           boost::edges(other.m_graph).first, boost::edges(other.m_graph).second,
	           [&](auto const& aa, auto const& bb) {
		           return (boost::source(aa, m_graph) == boost::source(bb, other.m_graph)) &&
		                  (boost::target(aa, m_graph) == boost::target(aa, other.m_graph));
	           }) &&
	       m_vertices == other.m_vertices && m_edges == other.m_edges;
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
bool Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::operator!=(
    Graph const& other) const
{
	return !(*this == other);
}

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
{
	hate::IndentingOstream ios(os);
	ios << hate::name<Derived>() << "(\n";
	ios << hate::Indentation("\t");
	for (auto const descriptor : boost::make_iterator_range(value.vertices())) {
		ios << descriptor << ": " << value.get(descriptor) << "\n";
	}
	for (auto const descriptor : boost::make_iterator_range(value.edges())) {
		ios << descriptor << ": " << value.get(descriptor) << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
    check_contains(VertexDescriptor const& descriptor, char const* description) const
{
	if (!contains(descriptor)) {
		std::stringstream ss;
		ss << "Trying to " << description << " vertex (" << descriptor
		   << "), which is not part of topology.";
		throw std::out_of_range(ss.str());
	}
}

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
    check_contains(EdgeDescriptor const& descriptor, char const* description) const
{
	if (!contains(descriptor)) {
		std::stringstream ss;
		ss << "Trying to " << description << " edge (" << descriptor
		   << "), which is not part of topology.";
		throw std::out_of_range(ss.str());
	}
}

} // namespace grenade::common
