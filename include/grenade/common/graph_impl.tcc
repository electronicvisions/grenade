#pragma once
#include "grenade/common/detail/descriptor_transform.h"
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
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::Graph() :
    m_backend(std::make_unique<Backend>()),
    m_vertices(),
    m_edges(),
    m_vertex_descriptors(),
    m_edge_descriptors()
{
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
    Graph const& other) :
    m_backend(std::make_unique<Backend>()),
    m_vertices(),
    m_edges(),
    m_vertex_descriptors(),
    m_edge_descriptors()
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
    m_backend(std::move(other.m_backend)),
    m_vertices(std::move(other.m_vertices)),
    m_edges(std::move(other.m_edges)),
    m_vertex_descriptors(std::move(other.m_vertex_descriptors)),
    m_edge_descriptors(std::move(other.m_edge_descriptors))
{
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
    Graph const& other)
{
	if (this != &other) {
		m_backend = std::make_unique<Backend>();
		m_vertices = other.m_vertices;
		m_edges = other.m_edges;
		for (auto const other_descriptor : boost::make_iterator_range(other.vertices())) {
			auto const backend_descriptor(boost::add_vertex(backend()));
			m_vertex_descriptors.insert({other_descriptor, backend_descriptor});
		}
		for (auto const other_descriptor : boost::make_iterator_range(other.edges())) {
			auto const [backend_descriptor, success] = boost::add_edge(
			    m_vertex_descriptors.left.at(other.source(other_descriptor)),
			    m_vertex_descriptors.left.at(other.target(other_descriptor)), backend());
			assert(success);
			m_edge_descriptors.insert({other_descriptor, backend_descriptor});
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
		m_backend = std::move(other.m_backend);
		m_vertices = std::move(other.m_vertices);
		m_edges = std::move(other.m_edges);
		m_vertex_descriptors = std::move(other.m_vertex_descriptors);
		m_edge_descriptors = std::move(other.m_edge_descriptors);
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
	VertexDescriptor const descriptor = m_vertices.insert(vertex);
	auto const backend_descriptor(boost::add_vertex(backend()));
	m_vertex_descriptors.insert({descriptor, backend_descriptor});
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
	VertexDescriptor const descriptor = m_vertices.insert(std::move(vertex));
	auto const backend_descriptor(boost::add_vertex(backend()));
	m_vertex_descriptors.insert({descriptor, backend_descriptor});
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
	EdgeDescriptor const descriptor = m_edges.insert(edge);
	auto const [backend_descriptor, success] = boost::add_edge(
	    m_vertex_descriptors.left.at(source), m_vertex_descriptors.left.at(target), backend());
	if (!success) {
		std::stringstream ss;
		ss << "Trying to add edge from source (" << source << ") to target (" << target
		   << " not successful, since edge is already present (" << descriptor
		   << ") and graph doesn't support multiple edges between the same vertices.";
		throw std::runtime_error(ss.str());
	}
	m_edge_descriptors.insert({descriptor, backend_descriptor});
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
	EdgeDescriptor const descriptor = m_edges.insert(std::move(edge));
	auto const [backend_descriptor, success] = boost::add_edge(
	    m_vertex_descriptors.left.at(source), m_vertex_descriptors.left.at(target), backend());
	if (!success) {
		std::stringstream ss;
		ss << "Trying to add edge from source (" << source << ") to target (" << target
		   << " not successful, since edge is already present (" << descriptor
		   << ") and graph doesn't support multiple edges between the same vertices.";
		throw std::runtime_error(ss.str());
	}
	m_edge_descriptors.insert({descriptor, backend_descriptor});
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
		auto const num_descriptors_removed = m_edge_descriptors.left.erase(descriptor);
		assert(num_descriptors_removed == 1);
	}
	boost::remove_edge(
	    m_vertex_descriptors.left.at(source), m_vertex_descriptors.left.at(target), backend());
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
	boost::remove_edge(m_edge_descriptors.left.at(descriptor), backend());
	m_edge_descriptors.left.erase(descriptor);
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
	boost::remove_vertex(m_vertex_descriptors.left.at(descriptor), backend());
	m_vertex_descriptors.left.erase(descriptor);
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
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::clear()
{
	for (auto vertex : boost::make_iterator_range(vertices())) {
		clear_vertex(vertex);
	}
	while (!m_vertices.empty()) {
		remove_vertex(m_vertices.begin()->first);
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
Vertex const& Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::get(
    VertexDescriptor const& descriptor) const
{
	check_contains(descriptor, "get");
	return m_vertices.get(descriptor);
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
	m_vertices.set(descriptor, vertex);
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
	m_vertices.set(descriptor, std::move(vertex));
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
	return m_edges.get(descriptor);
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
	m_edges.set(descriptor, edge);
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
	m_edges.set(descriptor, std::move(edge));
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
	auto const backend_vertices = boost::vertices(backend());
	return std::pair<VertexIterator, VertexIterator>{
	    {backend_vertices.first, detail::DescriptorTransform{&m_vertex_descriptors}},
	    {backend_vertices.second, detail::DescriptorTransform{&m_vertex_descriptors}}};
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
	auto const backend_edges = boost::edges(backend());
	return std::pair<EdgeIterator, EdgeIterator>{
	    {backend_edges.first, detail::DescriptorTransform{&m_edge_descriptors}},
	    {backend_edges.second, detail::DescriptorTransform{&m_edge_descriptors}}};
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
	auto const backend_adjacent_vertices =
	    boost::adjacent_vertices(m_vertex_descriptors.left.at(descriptor), backend());
	return std::pair<AdjacencyIterator, AdjacencyIterator>{
	    {backend_adjacent_vertices.first, detail::DescriptorTransform{&m_vertex_descriptors}},
	    {backend_adjacent_vertices.second, detail::DescriptorTransform{&m_vertex_descriptors}}};
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
	auto const backend_inv_adjacent_vertices =
	    boost::inv_adjacent_vertices(m_vertex_descriptors.left.at(descriptor), backend());
	return std::pair<InvAdjacencyIterator, InvAdjacencyIterator>{
	    {backend_inv_adjacent_vertices.first, detail::DescriptorTransform{&m_vertex_descriptors}},
	    {backend_inv_adjacent_vertices.second, detail::DescriptorTransform{&m_vertex_descriptors}}};
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
	auto const backend_out_edges =
	    boost::out_edges(m_vertex_descriptors.left.at(descriptor), backend());
	return std::pair<OutEdgeIterator, OutEdgeIterator>{
	    {backend_out_edges.first, detail::DescriptorTransform{&m_edge_descriptors}},
	    {backend_out_edges.second, detail::DescriptorTransform{&m_edge_descriptors}}};
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
	auto const backend_in_edges =
	    boost::in_edges(m_vertex_descriptors.left.at(descriptor), backend());
	return std::pair<InEdgeIterator, InEdgeIterator>{
	    {backend_in_edges.first, detail::DescriptorTransform{&m_edge_descriptors}},
	    {backend_in_edges.second, detail::DescriptorTransform{&m_edge_descriptors}}};
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
	return m_vertex_descriptors.right.at(
	    boost::source(m_edge_descriptors.left.at(descriptor), backend()));
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
	return m_vertex_descriptors.right.at(
	    boost::target(m_edge_descriptors.left.at(descriptor), backend()));
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
	return boost::out_degree(m_vertex_descriptors.left.at(descriptor), backend());
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
	return boost::in_degree(m_vertex_descriptors.left.at(descriptor), backend());
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
	return boost::num_vertices(backend());
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
	return boost::num_edges(backend());
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
	auto const backend_edge_range = boost::edge_range(
	    m_vertex_descriptors.left.at(source), m_vertex_descriptors.left.at(target), backend());
	return std::pair<OutEdgeIterator, OutEdgeIterator>{
	    {backend_edge_range.first, detail::DescriptorTransform{&m_edge_descriptors}},
	    {backend_edge_range.second, detail::DescriptorTransform{&m_edge_descriptors}}};
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
		std::map<typename Backend::vertex_descriptor, size_t> strongly_connected_component_coloring;
		std::map<typename Backend::vertex_descriptor, size_t> vertex_index_map;
		for (size_t vertex_index = 0;
		     auto const vertex_descriptor : boost::make_iterator_range(vertices())) {
			vertex_index_map.emplace(m_vertex_descriptors.left.at(vertex_descriptor), vertex_index);
			vertex_index++;
		}
		boost::strong_components(
		    backend(), boost::make_assoc_property_map(strongly_connected_component_coloring),
		    boost::vertex_index_map(boost::make_assoc_property_map(vertex_index_map)));
		std::map<VertexDescriptor, size_t> ret;
		for (auto const& [key, value] : strongly_connected_component_coloring) {
			ret.emplace(VertexDescriptor(m_vertex_descriptors.right.at(key)), value);
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
std::map<VertexDescriptor, size_t>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
    get_vertex_index_map() const
{
	std::map<VertexDescriptor, size_t> vertex_index;
	size_t i = 0;

	for (auto const& descriptor : boost::make_iterator_range(vertices())) {
		vertex_index[descriptor] = i;
		i++;
	}

	return vertex_index;
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
std::map<typename Backend::vertex_descriptor, size_t>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
    get_backend_vertex_index_map() const
{
	std::map<typename Backend::vertex_descriptor, size_t> vertex_index;
	size_t i = 0;

	for (auto const& descriptor : boost::make_iterator_range(boost::vertices(backend()))) {
		vertex_index[descriptor] = i;
		i++;
	}

	return vertex_index;
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
std::map<VertexDescriptor, VertexDescriptor>
Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::isomorphism(
    Graph const& other) const
{
	auto vertex_index_1 = get_backend_vertex_index_map();
	auto const vertex_index_2 = other.get_backend_vertex_index_map();
	std::map<VertexDescriptor, VertexDescriptor> vertex_mapping;
	std::vector<typename Backend::vertex_descriptor> f(num_vertices());

	/**
	 * case for one vertex
	 * isomorphism maps on 0 instead of vertex id
	 * should be fixed in boost-1.86.0
	 */
	if (num_vertices() == 1) {
		vertex_mapping.emplace(*(vertices().first), *(other.vertices().first));
		return vertex_mapping;
	}

	bool valid = boost::isomorphism(
	    backend(), other.backend(),
	    boost::isomorphism_map(boost::make_iterator_property_map(
	                               f.begin(), boost::make_assoc_property_map(vertex_index_1), f[0]))
	        .vertex_index1_map(boost::make_assoc_property_map(vertex_index_1))
	        .vertex_index2_map(boost::make_assoc_property_map(vertex_index_2)));

	if (!valid) {
		return vertex_mapping;
	}

	for (auto const& [key_1, value_1] : vertex_index_1) {
		vertex_mapping.emplace(
		    m_vertex_descriptors.right.at(key_1),
		    other.m_vertex_descriptors.right.at(f.at(value_1)));
	}

	return vertex_mapping;
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
bool Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::is_connected()
    const
{
	std::set<VertexDescriptor> marked_vertices;
	VertexDescriptor root = *(vertices().first);

	is_connected_rec(root, marked_vertices);

	return (marked_vertices.size() == num_vertices());
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
    is_connected_rec(
        VertexDescriptor const& vertex, std::set<VertexDescriptor>& marked_vertices) const
{
	marked_vertices.emplace(vertex);

	for (auto neighbour : boost::make_iterator_range(adjacent_vertices(vertex))) {
		if (!marked_vertices.contains(neighbour)) {
			is_connected_rec(neighbour, marked_vertices);
		}
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
bool Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::operator==(
    Graph const& other) const
{
	if ((num_vertices() != other.num_vertices()) || (num_edges() != other.num_edges())) {
		return false;
	}

	std::unordered_map<typename Backend::vertex_descriptor, typename Backend::vertex_descriptor>
	    vertex_descriptor_translation;
	auto vertices_it = boost::vertices(backend()).first;
	auto other_vertices_it = boost::vertices(other.backend()).first;
	for (size_t i = 0; i < num_vertices(); ++i) {
		if (m_vertex_descriptors.right.at(*vertices_it) !=
		    other.m_vertex_descriptors.right.at(*other_vertices_it)) {
			return false;
		}
		vertex_descriptor_translation.emplace(*vertices_it, *other_vertices_it);
		vertices_it++;
		other_vertices_it++;
	}

	if (!std::equal(
	        boost::edges(backend()).first, boost::edges(backend()).second,
	        boost::edges(other.backend()).first, boost::edges(other.backend()).second,
	        [&](auto const& aa, auto const& bb) {
		        return (m_edge_descriptors.right.at(aa) == other.m_edge_descriptors.right.at(bb)) &&
		               (vertex_descriptor_translation.at(boost::source(aa, backend())) ==
		                boost::source(bb, other.backend())) &&
		               (vertex_descriptor_translation.at(boost::target(aa, backend())) ==
		                boost::target(bb, other.backend()));
	        })) {
		return false;
	}

	return m_vertices == other.m_vertices && m_edges == other.m_edges;
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
		ios << descriptor << " (" << value.source(descriptor) << " -> " << value.target(descriptor)
		    << "): " << value.get(descriptor) << "\n";
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

template <
    typename Derived,
    typename Backend,
    typename Vertex,
    typename Edge,
    typename VertexDescriptor,
    typename EdgeDescriptor,
    template <typename...>
    typename Holder>
Backend& Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::backend()
    const
{
	if (!m_backend) {
		throw std::logic_error("Unexpected access to moved-from object.");
	}
	return *m_backend;
}

} // namespace grenade::common
