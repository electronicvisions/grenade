#include "grenade/vx/network/abstract/mapping/uncalibrated_synapse.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/projection.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/build_connection_weight_split.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"


namespace grenade::vx::network::abstract {

UncalibratedSynapseMapping::UncalibratedSynapseMapping(Translation translation, Labels labels) :
    translation(std::move(translation)), labels(std::move(labels))
{
}

bool UncalibratedSynapseMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	for (auto const& mapped_vertex_descriptor : linked_vertex_descriptors) {
		auto const& mapped_vertex = topology.get(mapped_vertex_descriptor);
		if (dynamic_cast<grenade::vx::signal_flow::vertex::SynapseArrayViewSparse const*>(
		        &mapped_vertex) == nullptr) {
			return false;
		}
	}
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const projection = dynamic_cast<grenade::common::Projection const*>(
	        &topology.get_reference().get(linked_vertex_descriptors.at(0)));
	    projection) {
		if (auto const synapse =
		        dynamic_cast<grenade::vx::network::abstract::UncalibratedSynapse const*>(
		            &projection->get_synapse());
		    synapse) {
			return true;
		}
	}
	return false;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
UncalibratedSynapseMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        model_vertex_parameterization,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::LinkedTopology const& topology) const
{
	auto const& partitioned_synapse_parameterization =
	    dynamic_cast<UncalibratedSynapse::ParameterSpace::Parameterization const&>(
	        model_vertex_parameterization.at(0).at(1).value().get());

	std::vector<signal_flow::vertex::SynapseArrayViewSparse::Parameterization::Weights> weights(
	    linked_vertex_descriptors.size());

	for (size_t i = 0; auto const& mapped_vertex_descriptor : linked_vertex_descriptors) {
		auto const& mapped_vertex =
		    dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse const&>(
		        topology.get(mapped_vertex_descriptor));

		weights.at(i).resize(std::distance(
		    mapped_vertex.get_synapses().begin(), mapped_vertex.get_synapses().end()));
		++i;
	}

	for (auto const& [synapse_on_partitioned, synapses_on_mapped] : translation) {
		auto const& partitioned_weight =
		    partitioned_synapse_parameterization.weights.at(synapse_on_partitioned);
		size_t num_mapped_synapses = 0;
		for (auto const& synapse_on_mapped : synapses_on_mapped) {
			num_mapped_synapses += synapse_on_mapped.size();
		}
		auto const mapped_weights =
		    build_connection_weight_split(partitioned_weight, num_mapped_synapses);
		size_t used_weight = 0;
		for (size_t i = 0; i < synapses_on_mapped.size(); ++i) {
			for (auto const& synapse_on_mapped : synapses_on_mapped.at(i)) {
				weights.at(i).at(synapse_on_mapped) = mapped_weights.at(used_weight);
				used_weight++;
			}
		}
	}

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> mapped_parameterizations;

	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		mapped_parameterizations.push_back({});
		mapped_parameterizations.back().emplace_back(nullptr);
		mapped_parameterizations.back().emplace_back(
		    std::make_unique<signal_flow::vertex::SynapseArrayViewSparse::Parameterization>(
		        labels.at(i), std::move(weights.at(i))));
	}

	return mapped_parameterizations;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> UncalibratedSynapseMapping::copy() const
{
	return std::make_unique<UncalibratedSynapseMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> UncalibratedSynapseMapping::move()
{
	return std::make_unique<UncalibratedSynapseMapping>(std::move(*this));
}

std::ostream& UncalibratedSynapseMapping::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "UncalibratedSynapseMapping(\n";
	ios << hate::Indentation("\t");
	ios << "translation:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& [synapse_on_projection, synapse_views] : translation) {
		ios << "synapse on projection " << synapse_on_projection << ":\n";
		ios << hate::Indentation("\t\t\t");
		for (size_t i = 0; i < synapse_views.size(); ++i) {
			ios << "synapse view " << i << ": " << hate::join(synapse_views.at(i), ", ") << "\n";
		}
	}
	ios << "labels:\n";
	ios << hate::Indentation("\t\t");
	for (size_t i = 0; i < labels.size(); ++i) {
		ios << "synapse view " << i << ":\n"
		    << hate::Indentation("\t\t\t") << hate::join(labels.at(i), "\n") << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool UncalibratedSynapseMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return InterTopologyHyperEdge::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
