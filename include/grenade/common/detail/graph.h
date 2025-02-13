#pragma once
#include <type_traits>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_selectors.hpp>

namespace grenade::common::detail {

/**
 * Bidirectional (multi-)graph.
 *
 * We use listS for the vertex descriptors, because we need descriptor stability across removal of
 * vertices, since they are used to access stored properties alongside the graph. In addition we
 * require construction order stability and therefore do not use a set.
 *
 * We use multisetS for the edge descriptors, because we need descriptor stability across removal of
 * edges since they are used to access stored properties alongside the graph.
 * We don't use listS because we want to support edge_range functionality.
 */
typedef boost::adjacency_list<boost::multisetS, boost::listS, boost::bidirectionalS>
    BidirectionalMultiGraph;

/**
 * Bidirectional graph.
 *
 * We use listS for the vertex descriptors, because we need descriptor stability across removal of
 * vertices, since they are used to access stored properties alongside the graph. In addition we
 * require construction order stability and therefore do not use a set.
 *
 * We use setS for the edge descriptors, because we need descriptor stability across removal of
 * edges since they are used to access stored properties alongside the graph.
 * We don't use listS because we want to support edge_range functionality.
 */
typedef boost::adjacency_list<boost::setS, boost::listS, boost::bidirectionalS> BidirectionalGraph;

/**
 * Undirected graph.
 *
 * We use listS for the vertex descriptors, because we need descriptor stability across removal of
 * vertices, since they are used to access stored properties alongside the graph. In addition we
 * require construction order stability and therefore do not use a set.
 *
 * We use setS for the edge descriptors, because we need descriptor stability across removal of
 * edges since they are used to access stored properties alongside the graph.
 * We don't use listS because we want to support edge_range functionality.
 */
typedef boost::adjacency_list<boost::setS, boost::listS, boost::undirectedS> UndirectedGraph;


template <typename T>
struct IsSupportedGraph : public std::false_type
{};

template <typename VertexCollection, typename EdgeCollection, typename Directionality>
struct IsSupportedGraph<boost::adjacency_list<EdgeCollection, VertexCollection, Directionality>>
{
	// clang-format off
	constexpr static bool value =
	    std::is_same_v<VertexCollection, boost::listS> &&
	    (std::is_same_v<EdgeCollection, boost::multisetS> ||
	     std::is_same_v<EdgeCollection, boost::setS>) &&
	    (std::is_same_v<Directionality, boost::bidirectionalS> ||
	     std::is_same_v<Directionality, boost::undirectedS>);
	// clang-format on
};

} // namespace grenade::common::detail

namespace std {

static_assert(std::is_same_v<
              grenade::common::detail::BidirectionalMultiGraph::edge_descriptor,
              grenade::common::detail::BidirectionalGraph::edge_descriptor>);

template <>
struct hash<grenade::common::detail::BidirectionalMultiGraph::edge_descriptor>
{
	size_t operator()(
	    grenade::common::detail::BidirectionalMultiGraph::edge_descriptor const& e) const
	{
		return boost::hash<grenade::common::detail::BidirectionalMultiGraph::edge_descriptor>{}(e);
	}
};

template <>
struct hash<grenade::common::detail::UndirectedGraph::edge_descriptor>
{
	size_t operator()(grenade::common::detail::UndirectedGraph::edge_descriptor const& e) const
	{
		return boost::hash<grenade::common::detail::UndirectedGraph::edge_descriptor>{}(e);
	}
};

} // namespace std
