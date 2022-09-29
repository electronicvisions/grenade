#pragma once
#include <boost/graph/adjacency_list.hpp>

namespace grenade::vx::signal_flow::detail {

/** Bidirectional graph. */
typedef boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::bidirectionalS,
    boost::no_property,
    boost::no_property>
    Graph;

typedef Graph::vertex_descriptor vertex_descriptor;
typedef Graph::edge_descriptor edge_descriptor;

} // namespace grenade::vx::signal_flow::detail

namespace std {

template <>
struct hash<grenade::vx::signal_flow::detail::Graph::edge_descriptor>
{
	size_t operator()(grenade::vx::signal_flow::detail::Graph::edge_descriptor const& e) const
	{
		return boost::hash<grenade::vx::signal_flow::detail::Graph::edge_descriptor>{}(e);
	}
};

} // namespace std
