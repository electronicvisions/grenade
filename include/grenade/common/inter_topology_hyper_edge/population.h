#pragma once
#include "grenade/common/genpybind.h"
#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/common/vertex.h"
#include "grenade/common/vertex_on_topology.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

struct LinkedTopology;


/**
 * Link between one population in reference topology and (possibly multiple) populations in linked
 * topology.
 */
struct SYMBOL_VISIBLE PopulationInterTopologyHyperEdge : InterTopologyHyperEdge
{
	PopulationInterTopologyHyperEdge() = default;

	virtual std::unique_ptr<InterTopologyHyperEdge> copy() const override;
	virtual std::unique_ptr<InterTopologyHyperEdge> move() override;

	/**
	 * Get whether edge is valid for the given linked vertices w.r.t. the given reference
	 * vertices.
	 * This is the case exactly if all reference and linked vertices are populations, there's
	 * exactly one reference vertex descriptor present and all linked populations' shapes are
	 * included in the reference topology population's shape.
	 */
	virtual bool valid(
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
	    LinkedTopology const& topology) const override;

	/**
	 * Map reference input data to link input data.
	 * For each linked topology population, the section of the shape w.r.t. the reference topology
	 * population's shape is extracted from the input data and returned.
	 *
	 * @param reference_vertex_input_data Input data of the reference vertices
	 * @param linked_vertex_descriptors Vertex descriptors of linked topology
	 * @param reference_vertex_descriptors Vertex descriptors of reference topology
	 * @param topology Topology
	 * @return Input data of linked vertices
	 * @throws std::invalid_argument On data not matching expected parameterization or dynamics of
	 * the reference topology population
	 */
	virtual std::vector<std::vector<std::unique_ptr<PortData>>> map_input_data(
	    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&
	        reference_vertex_input_data,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
	    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
	    LinkedTopology const& topology) const override;

protected:
	virtual bool is_equal_to(InterTopologyHyperEdge const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace common
} // namespace grenade
