#include "grenade/vx/network/abstract/mapping/uncalibrated_signed_synapse.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/projection.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated_signed.h"


namespace grenade::vx::network::abstract {

UncalibratedSignedSynapseMapping::UncalibratedSignedSynapseMapping() {}

bool UncalibratedSignedSynapseMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (linked_vertex_descriptors.size() != 2) {
		return false;
	}
	for (auto const& mapped_vertex_descriptor : linked_vertex_descriptors) {
		if (auto const projection = dynamic_cast<grenade::common::Projection const*>(
		        &topology.get(mapped_vertex_descriptor));
		    projection) {
			if (auto const synapse =
			        dynamic_cast<grenade::vx::network::abstract::UncalibratedSynapse const*>(
			            &projection->get_synapse());
			    !synapse) {
				return false;
			}
		}
	}

	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const projection = dynamic_cast<grenade::common::Projection const*>(
	        &topology.get_reference().get(reference_vertex_descriptors.at(0)));
	    projection) {
		if (auto const synapse =
		        dynamic_cast<grenade::vx::network::abstract::UncalibratedSignedSynapse const*>(
		            &projection->get_synapse());
		    synapse) {
			return true;
		}
	}
	return false;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
UncalibratedSignedSynapseMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        reference_vertex_input_data,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::LinkedTopology const&) const
{
	auto const& model_synapse_parameterization =
	    dynamic_cast<UncalibratedSignedSynapse::ParameterSpace::Parameterization const&>(
	        reference_vertex_input_data.at(0).at(1).value().get());

	std::vector<UncalibratedSynapse::Weight> new_exc_weights(model_synapse_parameterization.size());
	std::vector<UncalibratedSynapse::Weight> new_inh_weights(model_synapse_parameterization.size());
	for (size_t i = 0; i < model_synapse_parameterization.size(); ++i) {
		new_exc_weights.at(i) = UncalibratedSynapse::Weight(std::max(
		    UncalibratedSignedSynapse::Weight(0), model_synapse_parameterization.weights.at(i)));
		new_inh_weights.at(i) = UncalibratedSynapse::Weight(-std::min(
		    UncalibratedSignedSynapse::Weight(0), model_synapse_parameterization.weights.at(i)));
	}

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> mapped_parameterizations;

	mapped_parameterizations.push_back({});
	mapped_parameterizations.back().emplace_back(nullptr); // no data on spike input port
	mapped_parameterizations.back().emplace_back(
	    std::make_unique<UncalibratedSynapse::ParameterSpace::Parameterization>(
	        std::move(new_exc_weights)));
	mapped_parameterizations.push_back({});
	mapped_parameterizations.back().emplace_back(nullptr); // no data on spike input port
	mapped_parameterizations.back().emplace_back(
	    std::make_unique<UncalibratedSynapse::ParameterSpace::Parameterization>(
	        std::move(new_inh_weights)));

	return mapped_parameterizations;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> UncalibratedSignedSynapseMapping::copy()
    const
{
	return std::make_unique<UncalibratedSignedSynapseMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> UncalibratedSignedSynapseMapping::move()
{
	return std::make_unique<UncalibratedSignedSynapseMapping>(std::move(*this));
}

std::ostream& UncalibratedSignedSynapseMapping::print(std::ostream& os) const
{
	return os << "UncalibratedSignedSynapseMapping()";
}

bool UncalibratedSignedSynapseMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return InterTopologyHyperEdge::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
