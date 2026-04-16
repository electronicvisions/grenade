#pragma once
#include "dapr/map.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/inter_topology_hyper_edge.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Inter-topology hyper edge, which provides stored port data to an input port.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) FixtureInterTopologyHyperEdge
    : public grenade::common::InterTopologyHyperEdge
{
	typedef dapr::Map<size_t, PortData> PortDataPerPort;

	FixtureInterTopologyHyperEdge(PortDataPerPort port_data_per_port);
	FixtureInterTopologyHyperEdge(size_t input_port_on_vertex, PortData const& port_data);
	FixtureInterTopologyHyperEdge(size_t input_port_on_vertex, PortData&& port_data);

	PortDataPerPort port_data_per_port;

	/**
	 * Get whether edge is valid for the given linked vertices w.r.t. the given reference
	 * vertices.
	 * This is the case exactly if no reference vertices exist and all contained port data is for
	 * input ports, which are in the range of the linked vertices' number of input ports.
	 */
	virtual bool valid(
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& linked_vertex_descriptors,
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& reference_vertex_descriptors,
	    grenade::common::LinkedTopology const& topology) const override;

	/**
	 * Map reference input data to link input data.
	 * For each linked vertex, the contained port data is returned.
	 *
	 * @param reference_vertex_input_data Input data of the reference vertices
	 * @param linked_vertex_descriptors Vertex descriptors of linked topology
	 * @param reference_vertex_descriptors Vertex descriptors of reference topology
	 * @param topology Topology
	 * @return Input data of linked vertices
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
