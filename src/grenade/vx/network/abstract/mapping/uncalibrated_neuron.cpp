#include "grenade/vx/network/abstract/mapping/uncalibrated_neuron.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/population.h"
#include "grenade/vx/network/abstract/mapping/locally_placed_neuron.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/indent.h"


namespace grenade::vx::network::abstract {

UncalibratedNeuronMapping::UncalibratedNeuronMapping(Labels labels, Anchors anchors) :
    LocallyPlacedNeuronMapping(std::move(labels), std::move(anchors))
{
}

bool UncalibratedNeuronMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (!LocallyPlacedNeuronMapping::valid(
	        linked_vertex_descriptors, reference_vertex_descriptors, topology)) {
		return false;
	}
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const population = dynamic_cast<grenade::common::Population const*>(
	        &topology.get_reference().get(reference_vertex_descriptors.at(0)));
	    population) {
		if (auto const neuron =
		        dynamic_cast<grenade::vx::network::abstract::UncalibratedNeuron const*>(
		            &population->get_cell());
		    neuron) {
			return true;
		}
	}
	return false;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
UncalibratedNeuronMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        model_vertex_parameterization,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	auto const& model_neuron_parameterization =
	    dynamic_cast<UncalibratedNeuron::ParameterSpace::Parameterization const&>(
	        model_vertex_parameterization.at(0).at(1).value().get());

	auto const& model_neuron = dynamic_cast<UncalibratedNeuron const&>(
	    dynamic_cast<grenade::common::Population const&>(
	        topology.get_reference().get(reference_vertex_descriptors.at(0)))
	        .get_cell());

	std::vector<std::vector<signal_flow::vertex::NeuronView::Parameterization::Config>>
	    mapped_configs(linked_vertex_descriptors.size());

	for (size_t i = 0; auto const& mapped_vertex_descriptor : linked_vertex_descriptors) {
		auto const& mapped_vertex = dynamic_cast<signal_flow::vertex::NeuronView const&>(
		    topology.get(mapped_vertex_descriptor));

		mapped_configs.at(i).resize(mapped_vertex.get_columns().size());
		++i;
	}

	for (auto const& [neuron_on_model, anchors_on_mapped] : anchors) {
		auto const& model_config = model_neuron_parameterization.configs.at(neuron_on_model);
		auto const& anchor = anchors_on_mapped.second;
		auto const atomic_neurons_on_model =
		    halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(model_neuron.shape, anchor)
		        .get_placed_compartments();
		for (auto const& [hemisphere, mapped_vertex_index] : anchors_on_mapped.first) {
			auto const& mapped_vertex = dynamic_cast<signal_flow::vertex::NeuronView const&>(
			    topology.get(linked_vertex_descriptors.at(mapped_vertex_index)));
			for (auto const& [compartment_on_neuron, atomic_neurons] : atomic_neurons_on_model) {
				for (size_t i = 0; i < atomic_neurons.size(); ++i) {
					if (atomic_neurons.at(i).toNeuronRowOnDLS() == mapped_vertex.row) {
						size_t const column_index = std::distance(
						    mapped_vertex.get_columns().begin(),
						    std::find(
						        mapped_vertex.get_columns().begin(),
						        mapped_vertex.get_columns().end(),
						        atomic_neurons.at(i).toNeuronColumnOnDLS()));
						assert(column_index < mapped_vertex.get_columns().size());

						mapped_configs.at(mapped_vertex_index).at(column_index) = {
						    model_config
						        .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
						        .at(i),
						    false};
						if (labels.at(mapped_vertex_index).at(column_index)) {
							mapped_configs.at(mapped_vertex_index)
							    .at(column_index)
							    .atomic_neuron_config.event_routing.address =
							    *labels.at(mapped_vertex_index).at(column_index);
							mapped_configs.at(mapped_vertex_index)
							    .at(column_index)
							    .atomic_neuron_config.event_routing.enable_digital = true;
						} else {
							mapped_configs.at(mapped_vertex_index)
							    .at(column_index)
							    .atomic_neuron_config.event_routing.enable_digital = false;
						}
					}
				}
			}
		}
	}

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> mapped_parameterizations;

	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		mapped_parameterizations.push_back({});
		mapped_parameterizations.back().emplace_back(nullptr);
		mapped_parameterizations.back().emplace_back(
		    std::make_unique<signal_flow::vertex::NeuronView::Parameterization>(
		        std::move(mapped_configs.at(i))));
	}

	return mapped_parameterizations;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> UncalibratedNeuronMapping::copy() const
{
	return std::make_unique<UncalibratedNeuronMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> UncalibratedNeuronMapping::move()
{
	return std::make_unique<UncalibratedNeuronMapping>(std::move(*this));
}

std::ostream& UncalibratedNeuronMapping::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "UncalibratedNeuronMapping(\n";
	ios << hate::Indentation("\t");
	LocallyPlacedNeuronMapping::print(ios);
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool UncalibratedNeuronMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return LocallyPlacedNeuronMapping::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
