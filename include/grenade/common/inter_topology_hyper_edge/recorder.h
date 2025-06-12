#pragma once
#include "grenade/common/genpybind.h"
#include "grenade/common/inter_graph_hyper_edge_vertex_descriptors.h"
#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/common/vertex_on_topology.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

struct LinkedTopology;


/**
 * Link between reference and linked topology vertices.
 */
struct SYMBOL_VISIBLE RecorderInterTopologyHyperEdge : InterTopologyHyperEdge
{
	RecorderInterTopologyHyperEdge() = default;

	virtual std::unique_ptr<InterTopologyHyperEdge> copy() const override;
	virtual std::unique_ptr<InterTopologyHyperEdge> move() override;

	/**
	 * Get whether edge is valid for the given linked vertices w.r.t. the given reference
	 * vertices.
	 * This is the case exactly if all vertices are recorders and there's exactly one reference
	 * vertex descriptor.
	 */
	virtual bool valid(
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
	    LinkedTopology const& mapped_topology) const override;

	/**
	 * Map output_data from linked vertices to reference vertex.
	 * The given linked vertex output_data are expected to be valid for the
	 * respective vertex.
	 * For this, an empty result is created for the referencea recorder, which is filled with the
	 * linked recorders' results.
	 *
	 * @param linked_vertex_output_data Results of the linked vertex (results per port on vertex per
	 * linked vertex)
	 * @param topology Linked topology
	 * @return Results of reference vertex (result per port on vertex per reference vertex)
	 */
	virtual std::vector<std::vector<std::unique_ptr<PortData>>> map_output_data(
	    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&
	        linked_vertex_output_data,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
	    LinkedTopology const& topology) const override;

protected:
	virtual bool is_equal_to(InterTopologyHyperEdge const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace common
} // namespace grenade
