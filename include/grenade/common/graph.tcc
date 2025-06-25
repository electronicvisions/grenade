#pragma once
#include "grenade/common/graph.h"

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
template <typename Callback, typename VertexEquivalent>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::isomorphism(
    Graph const& other, Callback&& callback, VertexEquivalent&& vertex_equivalent) const
{
	// Mapping of vertices from this graph to other graph.
	std::map<VertexDescriptor, VertexDescriptor> vertex_mapping_max;
	// Maping of vertices from other graph to this graph.
	std::map<VertexDescriptor, VertexDescriptor> vertex_mapping_reverse_max;

	size_t unmapped_vertices_min = num_vertices();

	auto const vertex_mapping_callback = [this, &other, callback, &vertex_mapping_max,
	                                      &vertex_mapping_reverse_max,
	                                      &unmapped_vertices_min](auto f, auto) -> bool {
		// Mapping of vertices from this graph to other graph.
		std::map<VertexDescriptor, VertexDescriptor> vertex_mapping;
		// Maping of vertices from other graph to this graph.
		std::map<VertexDescriptor, VertexDescriptor> vertex_mapping_reverse;

		// Number of unmappable vertices
		size_t unmapped_vertices = 0;
		for (auto vertex : boost::make_iterator_range(other.vertices())) {
			if (boost::get(f, vertex.value()) == detail::UndirectedGraph::null_vertex()) {
				unmapped_vertices++;
			}
		}

		// Create mapping from this graph to other graph and reversed.
		vertex_mapping.clear();
		vertex_mapping_reverse.clear();
		for (auto vertex : boost::make_iterator_range(other.vertices())) {
			vertex_mapping[vertex] = VertexDescriptor(boost::get(f, vertex.value()));
			vertex_mapping_reverse[VertexDescriptor(boost::get(f, vertex.value()))] = vertex;
		}

		// Adjusts smallest number of unmappable vertices for all callbacks.
		if (unmapped_vertices < unmapped_vertices_min) {
			unmapped_vertices_min = unmapped_vertices;
			vertex_mapping_max = vertex_mapping;
			vertex_mapping_reverse_max = vertex_mapping_reverse;
		}

		// Return value determined by callback function.
		// If true search for other isomorphism continues.
		if (callback(unmapped_vertices_min, vertex_mapping, vertex_mapping_reverse)) {
			return true;
		} else {
			return false;
		}
	};

	std::vector<typename VertexDescriptor::Value> vertex_order;
	for (auto vertex : boost::make_iterator_range(other.vertices())) {
		vertex_order.push_back(vertex.value());
	}

	auto vertex_index_map_this = get_vertex_index_map();
	auto vertex_index_map_other = other.get_vertex_index_map();

	boost::vf2_graph_iso(
	    other.backend(), backend(), vertex_mapping_callback, vertex_order,
	    boost::vertex_index1_map(boost::make_assoc_property_map(vertex_index_map_other))
	        .vertex_index2_map(boost::make_assoc_property_map(vertex_index_map_this))
	        .vertices_equivalent(vertex_equivalent));
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
template <typename Callback, typename VertexEquivalent>
void Graph<Derived, Backend, Vertex, Edge, VertexDescriptor, EdgeDescriptor, Holder>::
    isomorphism_subgraph(
        Graph const& other, Callback&& callback, VertexEquivalent&& vertex_equivalent) const
{
	// Mapping of vertices from this graph to other graph.
	std::map<VertexDescriptor, VertexDescriptor> vertex_mapping_max;
	// Maping of vertices from other graph to this graph.
	std::map<VertexDescriptor, VertexDescriptor> vertex_mapping_reverse_max;

	size_t unmapped_vertices_min = num_vertices();

	auto const vertex_mapping_callback = [this, &other, callback, &vertex_mapping_max,
	                                      &vertex_mapping_reverse_max,
	                                      &unmapped_vertices_min](auto f, auto) -> bool {
		// Mapping of vertices from this graph to other graph.
		std::map<VertexDescriptor, VertexDescriptor> vertex_mapping;
		// Maping of vertices from other graph to this graph.
		std::map<VertexDescriptor, VertexDescriptor> vertex_mapping_reverse;

		// Number of unmappable vertices
		size_t unmapped_vertices = 0;
		for (auto vertex : boost::make_iterator_range(other.vertices())) {
			if (boost::get(f, vertex.value()) == detail::UndirectedGraph::null_vertex()) {
				unmapped_vertices++;
			}
		}

		// Create mapping from this graph to other graph and reversed.
		vertex_mapping.clear();
		vertex_mapping_reverse.clear();
		for (auto vertex : boost::make_iterator_range(other.vertices())) {
			vertex_mapping[vertex] = VertexDescriptor(boost::get(f, vertex.value()));
			vertex_mapping_reverse[VertexDescriptor(boost::get(f, vertex.value()))] = vertex;
		}

		// Adjusts smallest number of unmappable vertices for all callbacks.
		if (unmapped_vertices < unmapped_vertices_min) {
			unmapped_vertices_min = unmapped_vertices;
			vertex_mapping_max = vertex_mapping;
			vertex_mapping_reverse_max = vertex_mapping_reverse;
		}

		// Return value determined by callback function.
		// If true search for other isomorphism continues.
		if (callback(unmapped_vertices_min, vertex_mapping_max, vertex_mapping_reverse_max)) {
			return true;
		} else {
			return false;
		}
	};

	std::vector<typename VertexDescriptor::Value> vertex_order;
	for (auto vertex : boost::make_iterator_range(other.vertices())) {
		vertex_order.push_back(vertex.value());
	}

	auto vertex_index_map_this = get_vertex_index_map();
	auto vertex_index_map_other = other.get_vertex_index_map();

	boost::vf2_graph_iso(
	    other.backend(), backend(), vertex_mapping_callback, vertex_order,
	    boost::vertex_index1_map(boost::make_assoc_property_map(vertex_index_map_other))
	        .vertex_index2_map(boost::make_assoc_property_map(vertex_index_map_this))
	        .vertices_equivalent(vertex_equivalent));
}
} // namespace grenade::common