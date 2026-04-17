#pragma once
#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/vx/genpybind.h"
#include "halco/hicann-dls/vx/v3/event.h"
#include "hate/visibility.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Map from DelayCell population to SpikeLookup(s).
 */
struct SYMBOL_VISIBLE DelayCellMapping : public grenade::common::InterTopologyHyperEdge
{
	typedef std::map<
	    size_t,
	    std::pair<
	        halco::hicann_dls::vx::v3::SpikeLabel,
	        std::vector<halco::hicann_dls::vx::v3::SpikeLabel>>>
	    Labels;

	DelayCellMapping(Labels labels);

	Labels labels;

	virtual bool valid(
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& linked_vertex_descriptors,
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& reference_vertex_descriptors,
	    grenade::common::LinkedTopology const& topology) const override;

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
