#include "grenade/vx/network/abstract/mapping/calibrated_neuron.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/population.h"
#include "grenade/vx/network/abstract/mapping/locally_placed_neuron.h"
#include "grenade/vx/network/abstract/population_cell/calibrated.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/indent.h"


namespace grenade::vx::network::abstract {

CalibratedNeuronMapping::CalibratedNeuronMapping(Configs configs, Labels labels, Anchors anchors) :
    LocallyPlacedNeuronMapping(std::move(labels), std::move(anchors)), configs(std::move(configs))
{
}


lola::vx::v3::AtomicNeuron const& CalibratedNeuronMapping::get_config(
    size_t cell_on_population,
    grenade::common::CompartmentOnNeuron const& compartment_on_neuron,
    size_t atomic_neuron_on_compartment) const
{
	return configs.at(cell_on_population)
	    .at(compartment_on_neuron)
	    .at(atomic_neuron_on_compartment);
}

bool CalibratedNeuronMapping::valid(
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
	if (auto const population = dynamic_cast<grenade::common::Population const*>(
	        &dynamic_cast<grenade::common::LinkedTopology const&>(topology.get_reference())
	             .get(reference_vertex_descriptors.at(0)));
	    population) {
		if (auto const neuron =
		        dynamic_cast<grenade::vx::network::abstract::CalibratedNeuron const*>(
		            &population->get_cell());
		    neuron) {
			return true;
		}
	}
	return false;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
CalibratedNeuronMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	auto const& partitioned_neuron = dynamic_cast<CalibratedNeuron const&>(
	    dynamic_cast<grenade::common::Population const&>(
	        topology.get_reference().get(reference_vertex_descriptors.at(0)))
	        .get_cell());

	std::vector<std::vector<signal_flow::vertex::NeuronView::Parameterization::Config>>
	    mapped_configs(linked_vertex_descriptors.size());

	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		auto const& mapped_vertex = dynamic_cast<signal_flow::vertex::NeuronView const&>(
		    topology.get(linked_vertex_descriptors.at(i)));

		mapped_configs.at(i).resize(mapped_vertex.get_columns().size());
	}

	for (auto const& [neuron_on_model, anchors_on_mapped] : anchors) {
		auto const& anchor = anchors_on_mapped.second;
		auto const atomic_neurons_on_model =
		    halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(partitioned_neuron.shape, anchor)
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
						    configs.at(neuron_on_model)
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
		mapped_parameterizations.back().push_back(nullptr);
		mapped_parameterizations.back().emplace_back(
		    std::make_unique<signal_flow::vertex::NeuronView::Parameterization>(
		        std::move(mapped_configs.at(i))));
	}

	return mapped_parameterizations;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> CalibratedNeuronMapping::copy() const
{
	return std::make_unique<CalibratedNeuronMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> CalibratedNeuronMapping::move()
{
	return std::make_unique<CalibratedNeuronMapping>(std::move(*this));
}

std::ostream& CalibratedNeuronMapping::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "CalibratedNeuronMapping(\n";
	ios << hate::Indentation("\t");
	LocallyPlacedNeuronMapping::print(ios) << "\n";
	ios << "configs:\n";
	for (size_t cell_on_population = 0; cell_on_population < configs.size(); ++cell_on_population) {
		ios << hate::Indentation("\t\t");
		ios << "cell_on_population " << cell_on_population << ":\n";
		for (auto const& [compartment_on_neuron, cc] : configs.at(cell_on_population)) {
			ios << hate::Indentation("\t\t\t");
			ios << compartment_on_neuron << ":\n";
			ios << hate::Indentation("\t\t\t\t");
			for (size_t an = 0; an < cc.size(); ++an) {
				ios << "atomic_neuron_on_compartment " << an << ": " << cc.at(an) << "\n";
			}
		}
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool CalibratedNeuronMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return LocallyPlacedNeuronMapping::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
