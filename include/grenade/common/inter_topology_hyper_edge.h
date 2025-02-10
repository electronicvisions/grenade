#pragma once
#include "dapr/property.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/inter_graph_hyper_edge_vertex_descriptors.h"
#include "grenade/common/port_data.h"
#include "grenade/common/vertex.h"
#include "grenade/common/vertex_on_topology.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

struct LinkedTopology;


/**
 * Link between reference and linked topology vertices.
 */
struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) InterTopologyHyperEdge
    : public dapr::Property<InterTopologyHyperEdge>
{
	virtual ~InterTopologyHyperEdge();

	InterTopologyHyperEdge() = default;

	virtual std::unique_ptr<InterTopologyHyperEdge> copy() const override;
	virtual std::unique_ptr<InterTopologyHyperEdge> move() override;

	/**
	 * Get whether edge is valid for the given linked vertices w.r.t. the given reference
	 * vertices.
	 */
	virtual bool valid(
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
	    LinkedTopology const& topology) const;

	/**
	 * Map reference input data to link input data.
	 * The given reference vertex input data is expected to be valid for their
	 * respective vertices.
	 * By default, no input data is generated.
	 *
	 * @param reference_vertex_input_data Input data of the reference vertices
	 * @param linked_vertex_descriptors Vertex descriptors of linked topology
	 * @param reference_vertex_descriptors Vertex descriptors of reference topology
	 * @param topology Topology
	 * @return Input data of linked vertices
	 */
	virtual std::vector<std::vector<std::unique_ptr<PortData>>> map_input_data(
	    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&
	        reference_vertex_input_data,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
	    LinkedTopology const& topology) const;

	/**
	 * Map link output data to reference results.
	 * The given linked vertex output data are expected to be valid for the
	 * respective vertex.
	 * By default, no output data are generated.
	 *
	 * @param linked_vertex_output_data Output data of the linked vertex
	 * @param linked_vertex_descriptors Vertex descriptors of linked topology
	 * @param reference_vertex_descriptors Vertex descriptors of reference topology
	 * @param topology Topology
	 * @return Output data of reference vertices
	 */
	virtual std::vector<std::vector<std::unique_ptr<PortData>>> map_output_data(
	    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&
	        linked_vertex_output_data,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
	    LinkedTopology const& topology) const;

protected:
	virtual bool is_equal_to(InterTopologyHyperEdge const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace common
} // namespace grenade
