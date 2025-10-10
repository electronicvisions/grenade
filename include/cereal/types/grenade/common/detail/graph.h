#pragma once
#include "grenade/common/detail/graph.h"
#include "hate/type_list.h"
#include <cereal/types/map.hpp>

namespace cereal {

template <typename Archive, typename Graph>
std::enable_if_t<hate::is_in_type_list<
    Graph,
    hate::type_list<
        grenade::common::detail::BidirectionalMultiGraph,
        grenade::common::detail::BidirectionalGraph,
        grenade::common::detail::UndirectedGraph>>::value>
CEREAL_SAVE_FUNCTION_NAME(Archive& ar, Graph const& data)
{
	int const num_vertices = boost::num_vertices(data);
	int const num_edges = boost::num_edges(data);
	ar(CEREAL_NVP(num_vertices));
	ar(CEREAL_NVP(num_edges));
	// assign indices to vertices
	std::map<typename Graph::vertex_descriptor, int> indices;
	int num = 0;
	for (auto const vi : boost::make_iterator_range(boost::vertices(data))) {
		indices[vi] = num++;
	}
	// write edges
	for (auto const ei : boost::make_iterator_range(boost::edges(data))) {
		ar(CEREAL_NVP_("u", indices.at(boost::source(ei, data))));
		ar(CEREAL_NVP_("v", indices.at(boost::target(ei, data))));
	}
}
template <typename Archive, typename Graph>
std::enable_if_t<hate::is_in_type_list<
    Graph,
    hate::type_list<
        grenade::common::detail::BidirectionalMultiGraph,
        grenade::common::detail::BidirectionalGraph,
        grenade::common::detail::UndirectedGraph>>::value>
CEREAL_LOAD_FUNCTION_NAME(Archive& ar, Graph& data)
{
	int num_vertices;
	int num_edges;
	ar(CEREAL_NVP(num_vertices));
	ar(CEREAL_NVP(num_edges));
	std::vector<typename Graph::vertex_descriptor> vertices(num_vertices);
	int i = 0;
	while (num_vertices-- > 0) {
		auto const v = boost::add_vertex(data);
		vertices[i++] = v;
	}
	while (num_edges-- > 0) {
		int u;
		int v;
		ar(CEREAL_NVP(u));
		ar(CEREAL_NVP(v));
		[[maybe_unused]] auto const [_, inserted] =
		    boost::add_edge(vertices.at(u), vertices.at(v), data);
		assert(inserted);
	}
}
} // namespace cereal
