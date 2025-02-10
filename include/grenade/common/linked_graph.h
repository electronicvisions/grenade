#pragma once
#include "dapr/auto_key_map.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/graph.h"
#include "grenade/common/inter_graph_hyper_edge_vertex_descriptors.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>
#include <boost/iterator/transform_iterator.hpp>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Graph which allows linking of elements to a reference graph.
 * This enables e.g. keeping track of rewritten subgraphs and their corresponding
 * elements in the reference graph.
 */
template <
    typename InterGraphHyperEdgeDescriptorT,
    typename InterGraphHyperEdgeT,
    typename ReferenceGraphT,
    typename LinkGraphT>
struct SYMBOL_VISIBLE LinkedGraph : public LinkGraphT
{
	typedef ReferenceGraphT ReferenceGraph;
	typedef LinkGraphT LinkGraph;
	typedef InterGraphHyperEdgeDescriptorT InterGraphHyperEdgeDescriptor;
	typedef InterGraphHyperEdgeT InterGraphHyperEdge;

	typedef InterGraphHyperEdgeVertexDescriptors<typename ReferenceGraph::VertexDescriptor>
	    InterGraphHyperEdgeReferenceVertexDescriptors;
	typedef InterGraphHyperEdgeVertexDescriptors<typename LinkGraph::VertexDescriptor>
	    InterGraphHyperEdgeLinkVertexDescriptors;

	typedef boost::bimap<
	    boost::bimaps::unordered_multiset_of<
	        typename ReferenceGraph::VertexDescriptor,
	        std::hash<typename ReferenceGraph::VertexDescriptor>>,
	    boost::bimaps::unordered_multiset_of<
	        InterGraphHyperEdgeDescriptorT,
	        std::hash<InterGraphHyperEdgeDescriptorT>>>
	    ReferenceGraphInterGraphHyperEdgeMap;

	typedef boost::bimap<
	    boost::bimaps::unordered_multiset_of<
	        typename LinkGraph::VertexDescriptor,
	        std::hash<typename LinkGraph::VertexDescriptor>>,
	    boost::bimaps::unordered_multiset_of<
	        InterGraphHyperEdgeDescriptorT,
	        std::hash<InterGraphHyperEdgeDescriptorT>>>
	    LinkGraphInterGraphHyperEdgeMap;

	typedef dapr::AutoKeyMap<InterGraphHyperEdgeDescriptor, InterGraphHyperEdge>
	    InterGraphHyperEdges;

	struct GENPYBIND(hidden) PairFirstTransform
	{
		InterGraphHyperEdgeDescriptor operator()(auto const& it) const
		{
			return it.first;
		}
	};

	struct GENPYBIND(hidden) PairSecondTransform
	{
		InterGraphHyperEdgeDescriptor operator()(auto const& it) const
		{
			return it.second;
		}
	};

	typedef boost::transform_iterator<
	    PairSecondTransform,
	    typename ReferenceGraphInterGraphHyperEdgeMap::left_const_iterator>
	    LinkByReferenceIterator;

	typedef boost::transform_iterator<
	    PairSecondTransform,
	    typename LinkGraphInterGraphHyperEdgeMap::left_const_iterator>
	    LinkByLinkIterator;

	typedef boost::
	    transform_iterator<PairFirstTransform, typename InterGraphHyperEdges::ConstIterator>
	        InterGraphHyperEdgeIterator;

	// enable overloading of base-class methods
	using LinkGraph::contains;
	using LinkGraph::get;
	using LinkGraph::set;

	// enable overloading of base-class methods in Python
	GENPYBIND_MANUAL({
		typedef typename decltype(parent)::type self_type;
		parent.def(
		    "get",
		    [](GENPYBIND_PARENT_TYPE const& self,
		       typename self_type::VertexDescriptor const& descriptor)
		        -> std::shared_ptr<typename self_type::Vertex const> {
			    return self.get(descriptor).shared_from_this();
		    });
		parent.def(
		    "get", [](GENPYBIND_PARENT_TYPE const& self,
		              typename self_type::EdgeDescriptor const& descriptor) {
			    return self.get(descriptor);
		    });
		parent.def(
		    "set",
		    [](GENPYBIND_PARENT_TYPE& self, typename self_type::VertexDescriptor const& descriptor,
		       typename self_type::Vertex const& vertex) { return self.set(descriptor, vertex); });
		parent.def(
		    "set",
		    [](GENPYBIND_PARENT_TYPE& self, typename self_type::EdgeDescriptor const& descriptor,
		       typename self_type::Edge const& edge) { return self.set(descriptor, edge); });
		parent.def(
		    "contains", [](GENPYBIND_PARENT_TYPE const& self,
		                   typename self_type::VertexDescriptor const& descriptor) {
			    return self.contains(descriptor);
		    });
		parent.def(
		    "contains", [](GENPYBIND_PARENT_TYPE const& self,
		                   typename self_type::EdgeDescriptor const& descriptor) {
			    return self.contains(descriptor);
		    });
	})

	/**
	 * Construct linked graph with reference graph.
	 */
	LinkedGraph();

	LinkedGraph(LinkedGraph&&) = default;
	LinkedGraph(LinkedGraph const&);
	LinkedGraph& operator=(LinkedGraph&&) = default;
	LinkedGraph& operator=(LinkedGraph const&);

	/**
	 * Remove vertex.
	 * Also removes vertex from link mapping if present.
	 * @throws std::out_of_range On vertex not present in network
	 * @throws std::runtime_error On edges being present to or from vertex
	 * @param descriptor Vertex descriptor
	 */
	virtual void remove_vertex(typename LinkGraph::VertexDescriptor const& descriptor);

	/**
	 * Add inter-graph hyper edge between vertex and reference vertex groups.
	 * @param descriptor_group Descriptors to elements in linked graph
	 * @param reference_descriptor_group Descriptors to elements in reference graph
	 * @throws std::invalid_argument if any of descriptor_group or reference_descriptor_group are
	 * not part of their topologies
	 */
	InterGraphHyperEdgeDescriptor add_inter_graph_hyper_edge(
	    InterGraphHyperEdgeLinkVertexDescriptors const& descriptors,
	    InterGraphHyperEdgeReferenceVertexDescriptors const& reference_descriptors,
	    InterGraphHyperEdge const& inter_graph_hyper_edge);

	/**
	 * Set inter-graph hyper edge value.
	 * @param descriptor Descriptor to edge
	 * @param inter_graph_hyper_edge Inter-graph hyper edge value to set
	 * @throws std::out_of_range if descriptor is not part of the linked topology
	 */
	void set(
	    InterGraphHyperEdgeDescriptor const& descriptor,
	    InterGraphHyperEdge const& inter_graph_hyper_edge);

	/**
	 * Get whether linked graph contains inter-graph hyper edge for given descriptor.
	 * @param descriptor Descriptor to inter-graph hyper edge
	 */
	bool contains(InterGraphHyperEdgeDescriptor const& descriptor) const;

	/**
	 * Get inter-graph hyper edge value.
	 * @param descriptor Descriptor to element in linked graph
	 * @throws std::out_of_range if descriptor is not part of linked graph
	 */
	GENPYBIND(return_value_policy(reference_internal))
	InterGraphHyperEdge const& get(InterGraphHyperEdgeDescriptor const& descriptor) const;

	boost::iterator_range<InterGraphHyperEdgeIterator> inter_graph_hyper_edges() const
	    GENPYBIND(hidden);

	size_t num_inter_graph_hyper_edges() const;

	/**
	 * Get whether linked graph contains inter-graph hyper edge(s) for given descriptor.
	 * @param descriptor Descriptor to element in reference graph
	 * @throws std::out_of_range if descriptor is not part of graph
	 */
	bool contains_inter_graph_hyper_edge_to_reference(
	    typename ReferenceGraph::VertexDescriptor const& descriptor) const;

	/**
	 * Get whether linked graph contains inter-graph hyper edge(s) for given descriptor.
	 * @param descriptor Descriptor to element in linked graph
	 * @throws std::out_of_range if descriptor is not part of graph
	 */
	bool contains_inter_graph_hyper_edge_to_linked(
	    typename LinkGraph::VertexDescriptor const& descriptor) const;

	/**
	 * Get inter-graph hyper edge(s) between given reference graph descriptor and linked graph.
	 * This is the equaivalent operation to `out_edges()` on the reference graph for the inter-graph
	 * hyper edges.
	 * @param descriptor Descriptor to element in graph
	 * @throws std::out_of_range if no edges are present or descriptor is not part of graph
	 */
	boost::iterator_range<LinkByReferenceIterator> inter_graph_hyper_edges_by_reference(
	    typename ReferenceGraph::VertexDescriptor const& descriptor) const GENPYBIND(hidden);

	GENPYBIND_MANUAL({
		typedef typename decltype(parent)::type self_type;
		parent.def(
		    "inter_graph_hyper_edges_by_reference",
		    [](GENPYBIND_PARENT_TYPE const& self,
		       typename self_type::ReferenceGraph::VertexDescriptor const& descriptor) {
			    auto const iterators = self.inter_graph_hyper_edges_by_reference(descriptor);
			    return std::vector<typename self_type::InterGraphHyperEdgeDescriptor>(
			        iterators.begin(), iterators.end());
		    });
	})

	/**
	 * Get inter-graph hyper edge(s) between given linked graph descriptor and reference graph.
	 * This is the equaivalent operation to `in_edges()` on the linked graph for the inter-graph
	 * hyper edges.
	 * @param descriptor Descriptor to element in graph
	 * @throws std::out_of_range if no edges are present or descriptor is not part of graph
	 */
	boost::iterator_range<LinkByLinkIterator> inter_graph_hyper_edges_by_linked(
	    typename LinkGraph::VertexDescriptor const& descriptor) const GENPYBIND(hidden);

	GENPYBIND_MANUAL({
		typedef typename decltype(parent)::type self_type;
		parent.def(
		    "inter_graph_hyper_edges_by_linked",
		    [](GENPYBIND_PARENT_TYPE const& self,
		       typename self_type::LinkGraph::VertexDescriptor const& descriptor) {
			    auto const iterators = self.inter_graph_hyper_edges_by_linked(descriptor);
			    return std::vector<typename self_type::InterGraphHyperEdgeDescriptor>(
			        iterators.begin(), iterators.end());
		    });
	})

	/**
	 * Get linked graph vertex descriptors connected via inter-graph hyper edge.
	 * This is the equivalent operation to `target()` on a graph.
	 * @throws std::out_of_range if descriptor is not part of linked graph
	 */
	InterGraphHyperEdgeLinkVertexDescriptors const& links(
	    InterGraphHyperEdgeDescriptor const& descriptor) const;

	/**
	 * Get reference graph vertex descriptors connected via inter-graph hyper edge.
	 * This is the equivalent operation to `source()` on a graph.
	 * @throws std::out_of_range if descriptor is not part of linked graph
	 */
	InterGraphHyperEdgeReferenceVertexDescriptors const& references(
	    InterGraphHyperEdgeDescriptor const& descriptor) const;

	/**
	 * Remove inter-graph hyper edge.
	 * @throws std::out_of_range On edge not present in graph
	 * @param descriptor Vertex descriptor
	 */
	void remove_inter_graph_hyper_edge(InterGraphHyperEdgeDescriptor const& descriptor);

	/**
	 * Get whether linked graph is valid w.r.t. reference graph.
	 * This is the case exactly if
	 *  - for all elements a link exists to an element in the reference graph
	 *  - the reference graph is valid
	 *  - the linked graph is valid
	 */
	bool valid() const;

	/**
	 * Get reference graph.
	 */
	virtual ReferenceGraph const& get_reference() const = 0;

	void clear_all_inter_graph_hyper_edges();

private:
	InterGraphHyperEdges m_inter_graph_hyper_edges;
	std::unordered_map<InterGraphHyperEdgeDescriptor, InterGraphHyperEdgeLinkVertexDescriptors>
	    m_link_graph_descriptors;
	std::unordered_map<InterGraphHyperEdgeDescriptor, InterGraphHyperEdgeReferenceVertexDescriptors>
	    m_reference_graph_descriptors;
	ReferenceGraphInterGraphHyperEdgeMap m_reference_graph_link_map;
	LinkGraphInterGraphHyperEdgeMap m_link_graph_link_map;
};

} // namespace common
} // namespace grenade
