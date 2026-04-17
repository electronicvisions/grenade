#pragma once
#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include "lola/vx/v3/synapse.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Map from UncalibratedSynapse projection to SynapseArrayViewSparse vertices.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) UncalibratedSynapseMapping
    : public grenade::common::InterTopologyHyperEdge
{
	typedef std::map<size_t, std::vector<std::vector<size_t>>> Translation;
	typedef std::vector<std::vector<lola::vx::v3::SynapseMatrix::Label>> Labels;

	UncalibratedSynapseMapping(Translation translation, Labels labels);

	Translation translation;
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
