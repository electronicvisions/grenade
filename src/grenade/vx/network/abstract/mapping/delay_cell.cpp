#include "grenade/vx/network/abstract/mapping/delay_cell.h"

#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/common/linked_topology.h"
#include "grenade/vx/network/abstract/population_cell/delay.h"
#include "grenade/vx/network/abstract/recorder/spike.h"
#include "grenade/vx/network/vertex/transformation/spike_lookup.h"
#include "hate/indent.h"

namespace grenade::vx::network::abstract {

DelayCellMapping::DelayCellMapping(Labels labels) : labels(std::move(labels)) {}

bool DelayCellMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const population_ptr = dynamic_cast<grenade::common::Population const*>(
	        &topology.get_reference().get(reference_vertex_descriptors.at(0)));
	    population_ptr) {
		if (auto const synapse_ptr = dynamic_cast<DelayCell const*>(&population_ptr->get_cell());
		    synapse_ptr) {
			return true;
		}
		return false;
	} else {
		return false;
	}
	if (linked_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const transformation_ptr =
	        dynamic_cast<grenade::vx::signal_flow::vertex::Transformation const*>(
	            &topology.get(linked_vertex_descriptors.at(0)));
	    transformation_ptr) {
		return dynamic_cast<grenade::vx::network::vertex::transformation::SpikeLookup const*>(
		           &transformation_ptr->get_function()) != nullptr;
	}
	return false;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
DelayCellMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        model_vertex_parameterization,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::LinkedTopology const&) const
{
	auto const& model_neuron_parameterization =
	    dynamic_cast<DelayCell::ParameterSpace::Parameterization const&>(
	        model_vertex_parameterization.at(0).at(1).value().get());

	network::vertex::transformation::SpikeLookup::Parameterization::Translation translation;
	for (auto const& [cell_on_population, local_labels] : labels) {
		auto const& [source_label, target_labels] = local_labels;
		for (auto const& target_label : target_labels) {
			translation[source_label].push_back(
			    {target_label, model_neuron_parameterization.delays.at(cell_on_population)});
		}
	}

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> mapped_parameterizations;
	mapped_parameterizations.push_back({});
	mapped_parameterizations.back().emplace_back(nullptr);
	mapped_parameterizations.back().emplace_back(
	    std::make_unique<network::vertex::transformation::SpikeLookup::Parameterization>(
	        std::move(translation)));

	return mapped_parameterizations;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> DelayCellMapping::copy() const
{
	return std::make_unique<DelayCellMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> DelayCellMapping::move()
{
	return std::make_unique<DelayCellMapping>(std::move(*this));
}

std::ostream& DelayCellMapping::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "DelayCellMapping(\n";
	ios << hate::Indentation("\t");
	ios << "labels:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& [cell_on_population, local_labels] : labels) {
		auto const& [source_label, target_labels] = local_labels;
		ios << "cell_on_population " << cell_on_population << ": " << source_label << " -> ["
		    << hate::join(target_labels, ", ") << "]\n";
	}
	ios << hate::Indentation() << ")";
	return os;
}

bool DelayCellMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return labels == static_cast<DelayCellMapping const&>(other).labels;
}

} // namespace grenade::vx::network::abstract
