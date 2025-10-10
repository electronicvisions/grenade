#pragma once
#include "grenade/common/graph.h"

#include "cereal/types/dapr/auto_key_map.h"
#include "cereal/types/dapr/map.h"
#include "cereal/types/grenade/common/detail/graph.h"
#include <cereal/types/memory.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>

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
template <typename Archive>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::save(
    Archive& ar, std::uint32_t) const
{
	ar(m_backend);
	ar(m_vertices);
	ar(m_edges);
	// Manual serialization of descriptor mappings because they are not invariant under
	// serialization, but their ordered sequence is.
	std::vector<VertexDescriptor> vertex_descriptors;
	for (auto const& backend_descriptor : boost::make_iterator_range(boost::vertices(*m_backend))) {
		vertex_descriptors.push_back(m_vertex_descriptors.right.at(backend_descriptor));
	}
	ar(vertex_descriptors);
	std::vector<EdgeDescriptor> edge_descriptors;
	for (auto const& backend_descriptor : boost::make_iterator_range(boost::edges(*m_backend))) {
		edge_descriptors.push_back(m_edge_descriptors.right.at(backend_descriptor));
	}
	ar(edge_descriptors);
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
template <typename Archive>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::load(
    Archive& ar, std::uint32_t)
{
	ar(m_backend);
	ar(m_vertices);
	ar(m_edges);
	// Manual serialization of descriptor mappings because they are not invariant under
	// serialization, but their ordered sequence is.
	std::vector<VertexDescriptor> vertex_descriptors;
	ar(vertex_descriptors);
	for (size_t i = 0;
	     auto const& backend_descriptor : boost::make_iterator_range(boost::vertices(*m_backend))) {
		m_vertex_descriptors.insert({vertex_descriptors.at(i), backend_descriptor});
		i++;
	}
	std::vector<EdgeDescriptor> edge_descriptors;
	ar(edge_descriptors);
	for (size_t i = 0;
	     auto const& backend_descriptor : boost::make_iterator_range(boost::edges(*m_backend))) {
		m_edge_descriptors.insert({edge_descriptors.at(i), backend_descriptor});
		i++;
	}
}

} // namespace cereal
