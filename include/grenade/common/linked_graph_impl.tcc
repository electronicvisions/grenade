#include "grenade/common/linked_graph.h"

#include <memory>
#include <stdexcept>

namespace grenade::common {

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    LinkedGraph()
{
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    LinkedGraph(LinkedGraph const& other)
{
	operator=(other);
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>&
LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
operator=(LinkedGraph const& other)
{
	if (this != &other) {
		LinkGraph::operator=(other);
		m_inter_graph_hyper_edges = other.m_inter_graph_hyper_edges;
		m_reference_graph_descriptors = other.m_reference_graph_descriptors;
		m_link_graph_descriptors = other.m_link_graph_descriptors;
		m_link_graph_link_map = other.m_link_graph_link_map;
		m_reference_graph_link_map = other.m_reference_graph_link_map;
	}
	return *this;
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
void LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    remove_vertex(typename LinkGraph::VertexDescriptor const& descriptor)
{
	LinkGraph::remove_vertex(descriptor);
	auto range = m_link_graph_link_map.left.equal_range(descriptor);
	std::vector<InterGraphHyperEdgeDescriptor> inter_graph_hyper_edge_descriptors;
	for (auto const& [_, inter_graph_hyper_edge_descriptor] : boost::make_iterator_range(range)) {
		inter_graph_hyper_edge_descriptors.push_back(inter_graph_hyper_edge_descriptor);
	}
	for (auto const& inter_graph_hyper_edge_descriptor : inter_graph_hyper_edge_descriptors) {
		remove_inter_graph_hyper_edge(inter_graph_hyper_edge_descriptor);
	}
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
InterGraphHyperEdgeDescriptor
LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    add_inter_graph_hyper_edge(
        InterGraphHyperEdgeLinkVertexDescriptors const& descriptors,
        InterGraphHyperEdgeReferenceVertexDescriptors const& reference_descriptors,
        InterGraphHyperEdge const& inter_graph_hyper_edge)
{
	for (auto const& descriptor : descriptors) {
		if (!contains(descriptor)) {
			std::stringstream ss;
			ss << "Trying to add inter-graph hyper edge from " << descriptor
			   << ", which is not part of linked graph.";
			throw std::out_of_range(ss.str());
		}
	}
	for (auto const& descriptor : reference_descriptors) {
		if (!get_reference().contains(descriptor)) {
			std::stringstream ss;
			ss << "Trying to add inter-graph hyper edge to " << descriptor
			   << ", which is not part of reference graph.";
			throw std::out_of_range(ss.str());
		}
	}
	auto const inter_graph_hyper_edge_descriptor =
	    m_inter_graph_hyper_edges.insert(inter_graph_hyper_edge);

	for (auto const& descriptor : descriptors) {
		m_link_graph_link_map.insert({descriptor, inter_graph_hyper_edge_descriptor});
	}
	for (auto const& descriptor : reference_descriptors) {
		m_reference_graph_link_map.insert({descriptor, inter_graph_hyper_edge_descriptor});
	}
	m_reference_graph_descriptors.emplace(inter_graph_hyper_edge_descriptor, reference_descriptors);
	m_link_graph_descriptors.emplace(inter_graph_hyper_edge_descriptor, descriptors);
	return inter_graph_hyper_edge_descriptor;
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
void LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    set(InterGraphHyperEdgeDescriptor const& descriptor,
        InterGraphHyperEdge const& inter_graph_hyper_edge)
{
	if (!contains(descriptor)) {
		throw std::out_of_range("No inter-graph hyper edge present for given descriptor.");
	}
	m_inter_graph_hyper_edges.set(descriptor, inter_graph_hyper_edge);
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
bool LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    contains(InterGraphHyperEdgeDescriptor const& descriptor) const
{
	return m_inter_graph_hyper_edges.contains(descriptor);
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
InterGraphHyperEdge const&
LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::get(
    InterGraphHyperEdgeDescriptor const& descriptor) const
{
	if (!contains(descriptor)) {
		throw std::out_of_range("No inter-graph hyper edge present for given descriptor.");
	}
	return m_inter_graph_hyper_edges.get(descriptor);
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
boost::iterator_range<typename LinkedGraph<
    InterGraphHyperEdgeDescriptor,
    InterGraphHyperEdge,
    ReferenceGraph,
    LinkGraph>::InterGraphHyperEdgeIterator>
LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    inter_graph_hyper_edges() const
{
	return boost::iterator_range<InterGraphHyperEdgeIterator>{
	    m_inter_graph_hyper_edges.begin(), m_inter_graph_hyper_edges.end()};
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
size_t LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    num_inter_graph_hyper_edges() const
{
	return m_inter_graph_hyper_edges.size();
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
bool LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    contains_inter_graph_hyper_edge_to_reference(
        typename ReferenceGraph::VertexDescriptor const& descriptor) const
{
	if (!get_reference().contains(descriptor)) {
		throw std::out_of_range(
		    "Trying to get whether inter-graph hyper edges to reference vertex are contained in "
		    "linked graph, but reference vertex descriptor is not part of reference graph.");
	}
	return m_reference_graph_link_map.left.find(descriptor) !=
	       m_reference_graph_link_map.left.end();
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
bool LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    contains_inter_graph_hyper_edge_to_linked(
        typename LinkGraph::VertexDescriptor const& descriptor) const
{
	if (!contains(descriptor)) {
		throw std::out_of_range(
		    "Trying to get whether inter-graph hyper edges to linked vertex are contained in "
		    "linked graph, but linked vertex descriptor is not part of linked graph.");
	}
	return m_link_graph_link_map.left.find(descriptor) != m_link_graph_link_map.left.end();
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
boost::iterator_range<typename LinkedGraph<
    InterGraphHyperEdgeDescriptor,
    InterGraphHyperEdge,
    ReferenceGraph,
    LinkGraph>::LinkByReferenceIterator>
LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    inter_graph_hyper_edges_by_reference(
        typename ReferenceGraph::VertexDescriptor const& descriptor) const
{
	if (!get_reference().contains(descriptor)) {
		throw std::out_of_range(
		    "Trying to get inter-graph hyper edges to reference vertex, but reference vertex "
		    "descriptor is not part of reference graph.");
	}
	auto [begin, end] = m_reference_graph_link_map.left.equal_range(descriptor);
	return boost::iterator_range<LinkByReferenceIterator>{begin, end};
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
boost::iterator_range<typename LinkedGraph<
    InterGraphHyperEdgeDescriptor,
    InterGraphHyperEdge,
    ReferenceGraph,
    LinkGraph>::LinkByLinkIterator>
LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    inter_graph_hyper_edges_by_linked(typename LinkGraph::VertexDescriptor const& descriptor) const
{
	if (!contains(descriptor)) {
		throw std::out_of_range("Trying to get inter-graph hyper edges to linked vertex, but "
		                        "linked vertex descriptor is not part of linked graph.");
	}
	auto [begin, end] = m_link_graph_link_map.left.equal_range(descriptor);
	return boost::iterator_range<LinkByLinkIterator>{begin, end};
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
typename LinkedGraph<
    InterGraphHyperEdgeDescriptor,
    InterGraphHyperEdge,
    ReferenceGraph,
    LinkGraph>::InterGraphHyperEdgeLinkVertexDescriptors const&
LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::links(
    InterGraphHyperEdgeDescriptor const& descriptor) const
{
	if (!contains(descriptor)) {
		throw std::out_of_range("No inter-graph hyper edge present for given descriptor.");
	}
	return m_link_graph_descriptors.at(descriptor);
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
typename LinkedGraph<
    InterGraphHyperEdgeDescriptor,
    InterGraphHyperEdge,
    ReferenceGraph,
    LinkGraph>::InterGraphHyperEdgeReferenceVertexDescriptors const&
LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    references(InterGraphHyperEdgeDescriptor const& descriptor) const
{
	if (!contains(descriptor)) {
		throw std::out_of_range("No inter-graph hyper edge present for given descriptor.");
	}
	return m_reference_graph_descriptors.at(descriptor);
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
void LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    remove_inter_graph_hyper_edge(InterGraphHyperEdgeDescriptor const& descriptor)
{
	if (!contains(descriptor)) {
		throw std::out_of_range("No inter-graph hyper edge present for given descriptor.");
	}
	m_inter_graph_hyper_edges.erase(descriptor);
	auto reference_range = m_reference_graph_link_map.right.equal_range(descriptor);
	m_reference_graph_link_map.right.erase(reference_range.first, reference_range.second);
	auto link_range = m_link_graph_link_map.right.equal_range(descriptor);
	m_link_graph_link_map.right.erase(link_range.first, link_range.second);
	m_reference_graph_descriptors.erase(descriptor);
	m_link_graph_descriptors.erase(descriptor);
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
bool LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    valid() const
{
	auto const reference_vertices = get_reference().vertices();
	return LinkGraph::valid() &&
	       std::all_of(
	           reference_vertices.begin(), reference_vertices.end(),
	           [this](auto const& descriptor) {
		           return contains_inter_graph_hyper_edge_to_reference(descriptor);
	           }) &&
	       get_reference().valid();
}

template <
    typename InterGraphHyperEdgeDescriptor,
    typename InterGraphHyperEdge,
    typename ReferenceGraph,
    typename LinkGraph>
void LinkedGraph<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge, ReferenceGraph, LinkGraph>::
    clear_all_inter_graph_hyper_edges()
{
	m_inter_graph_hyper_edges.clear();
	m_link_graph_link_map.clear();
	m_reference_graph_link_map.clear();
	m_reference_graph_descriptors.clear();
	m_link_graph_descriptors.clear();
}

} // namespace grenade::common
