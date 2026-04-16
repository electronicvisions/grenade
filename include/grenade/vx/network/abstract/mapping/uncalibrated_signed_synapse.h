#pragma once
#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Mapping from a projection with uncalibrated signed synapse type to two projections with
 * uncalibrated synapse type.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) UncalibratedSignedSynapseMapping
    : public grenade::common::InterTopologyHyperEdge
{
	UncalibratedSignedSynapseMapping();

	/**
	 * Get whether edge is valid for the given linked vertices w.r.t. the given reference
	 * vertices.
	 * This is the case exactly if all reference and linked vertices are projections, there's
	 * exactly one reference vertex descriptor present, which is a projection with uncalibrated
	 * signed synapse type and two linked vertex descriptors, which with uncalibrated synapse type.
	 */
	virtual bool valid(
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& linked_vertex_descriptors,
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& reference_vertex_descriptors,
	    grenade::common::LinkedTopology const& topology) const override;

	/**
	 * Map reference input data to link input data.
	 * The weight parameter of the signed synapse is split into excitatory and inhibitory part and
	 * provided to the two unsigned projections.
	 *
	 * @param reference_vertex_input_data Input data of the reference vertices
	 * @param linked_vertex_descriptors Vertex descriptors of linked topology
	 * @param reference_vertex_descriptors Vertex descriptors of reference topology
	 * @param topology Topology
	 * @return Input data of linked vertices
	 * @throws std::invalid_argument On data not matching expected parameterization of
	 * the reference topology projection
	 */
	virtual std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> map_input_data(
	    std::vector<std::vector<
	        std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
	        reference_vertex_input_data,
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& linked_vertex_descriptors,
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& reference_vertex_descriptors,
	    grenade::common::LinkedTopology const& topology) const override;

	virtual std::unique_ptr<InterTopologyHyperEdge> copy() const override;
	virtual std::unique_ptr<InterTopologyHyperEdge> move() override;

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(InterTopologyHyperEdge const& other) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
