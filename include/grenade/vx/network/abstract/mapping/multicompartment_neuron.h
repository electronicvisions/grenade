#pragma once
#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/population_cell/calibrated.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <memory>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

struct SYMBOL_VISIBLE GENPYBIND(visible) MulticompartmentNeuronMapping
    : grenade::common::InterTopologyHyperEdge
{
	typedef CalibratedNeuron::ParameterSpace::Parameterization::ReadoutSources ReadoutSources;

	ReadoutSources readout_sources;

	MulticompartmentNeuronMapping(ReadoutSources readout_sources);


	virtual bool valid(
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& linked_vertex_descriptors,
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& reference_vertex_descriptors,
	    grenade::common::LinkedTopology const& mapped_topology) const override;

	virtual std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> map_input_data(
	    std::vector<std::vector<
	        std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
	        model_vertex_parameterizations,
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& linked_vertex_descriptors,
	    grenade::common::InterGraphHyperEdgeVertexDescriptors<
	        grenade::common::VertexOnTopology> const& reference_vertex_descriptors,
	    grenade::common::LinkedTopology const& mapped_topology) const override;

	virtual std::unique_ptr<InterTopologyHyperEdge> copy() const override;
	virtual std::unique_ptr<InterTopologyHyperEdge> move() override;

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(InterTopologyHyperEdge const& other) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
