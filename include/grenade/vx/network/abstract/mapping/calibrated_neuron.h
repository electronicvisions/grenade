#pragma once
#include "grenade/common/compartment_on_neuron.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/mapping/locally_placed_neuron.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <memory>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Map from CalibratedNeuron population to NeuronView(s).
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) CalibratedNeuronMapping : public LocallyPlacedNeuronMapping
{
	typedef std::vector<
	    std::map<grenade::common::CompartmentOnNeuron, std::vector<lola::vx::v3::AtomicNeuron>>>
	    Configs;

	CalibratedNeuronMapping(Configs configs, Labels labels, Anchors anchors);

	Configs configs;

	GENPYBIND(return_value_policy(reference_internal))
	lola::vx::v3::AtomicNeuron const& get_config(
	    size_t cell_on_population,
	    grenade::common::CompartmentOnNeuron const& compartment_on_neuron,
	    size_t atomic_neuron_on_compartment) const;

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
