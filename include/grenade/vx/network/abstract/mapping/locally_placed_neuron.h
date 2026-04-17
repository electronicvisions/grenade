#pragma once
#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/vx/genpybind.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <memory>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Base class for mapping from LocallyPlacedNeuron population to NeuronView(s).
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) LocallyPlacedNeuronMapping
    : public grenade::common::InterTopologyHyperEdge
{
	typedef std::vector<
	    std::vector<std::optional<lola::vx::v3::AtomicNeuron::EventRouting::Address>>>
	    Labels;
	// anchors per model neuron paired with mapped vertex indices
	typedef std::map<
	    size_t,
	    std::pair<
	        std::map<halco::hicann_dls::vx::v3::HemisphereOnDLS, size_t>,
	        halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>>
	    Anchors;

	LocallyPlacedNeuronMapping(Labels labels, Anchors anchors);

	Labels labels;
	Anchors anchors;

	virtual bool valid(
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
