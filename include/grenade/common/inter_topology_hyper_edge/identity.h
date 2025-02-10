#pragma once
#include "grenade/common/genpybind.h"
#include "grenade/common/inter_graph_hyper_edge_vertex_descriptors.h"
#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/common/vertex.h"
#include "grenade/common/vertex_on_topology.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

struct LinkedTopology;


/**
 * Identity inter-topology hyper edge.
 * All mappings between reference and linked topology for input_data, parameterization and
 * output_data perform an identity operation on the values.
 */
struct SYMBOL_VISIBLE IdentityInterTopologyHyperEdge : InterTopologyHyperEdge
{
	IdentityInterTopologyHyperEdge() = default;

	virtual std::unique_ptr<InterTopologyHyperEdge> copy() const override;
	virtual std::unique_ptr<InterTopologyHyperEdge> move() override;

	/**
	 * Get whether edge is valid for the given linked vertices w.r.t. the given reference
	 * vertices.
	 */
	virtual bool valid(
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
	    LinkedTopology const& topology) const override;

	/**
	 * Map reference input_data to link input_data.
	 * The given reference vertex input_data are expected to be valid for their
	 * respective vertices.
	 * An identity mapping is applied to the reference input_data.
	 *
	 * @param reference_vertex_input_data Dynamics of the reference vertices
	 * @param linked_vertex_descriptors Vertex descriptors of linked topology
	 * @param reference_vertex_descriptors Vertex descriptors of reference topology
	 * @param topology Topology
	 * @return Dynamics of linked vertices
	 */
	virtual std::vector<std::vector<std::unique_ptr<PortData>>> map_input_data(
	    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&
	        reference_vertex_input_data,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
	    LinkedTopology const& topology) const override;

	/**
	 * Map link output_data to reference output_data.
	 * The given linked vertex output_data are expected to be valid for the
	 * respective vertex.
	 * An identity mapping is applied to the link output_data.
	 *
	 * @param linked_vertex_output_data Results of the linked vertex
	 * @param linked_vertex_descriptors Vertex descriptors of linked topology
	 * @param reference_vertex_descriptors Vertex descriptors of reference topology
	 * @param topology Topology
	 * @return Results of reference vertices
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
