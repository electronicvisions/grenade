#include "grenade/vx/network/abstract/mapping/multicompartment_neuron.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/population.h"
#include "grenade/vx/network/abstract/mapping/locally_placed_neuron.h"
#include "grenade/vx/network/abstract/multicompartment/neuron.h"
#include "grenade/vx/network/abstract/population_cell/calibrated.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/indent.h"


namespace grenade::vx::network::abstract {

MulticompartmentNeuronMapping::MulticompartmentNeuronMapping(ReadoutSources readout_sources) :
    readout_sources(std::move(readout_sources))
{
}

bool MulticompartmentNeuronMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& mapped_topology) const
{
	if (linked_vertex_descriptors.size() != 1) {
		return false;
	}
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const population = dynamic_cast<grenade::common::Population const*>(
	        &dynamic_cast<grenade::common::LinkedTopology const&>(mapped_topology.get_reference())
	             .get(reference_vertex_descriptors.at(0)));
	    population) {
		if (auto const neuron = dynamic_cast<grenade::vx::network::abstract::Neuron const*>(
		        &population->get_cell());
		    !neuron) {
			return false;
		}
	} else {
		return false;
	}
	if (auto const population = dynamic_cast<grenade::common::Population const*>(
	        &dynamic_cast<grenade::common::LinkedTopology const&>(mapped_topology)
	             .get(linked_vertex_descriptors.at(0)));
	    population) {
		if (auto const neuron =
		        dynamic_cast<grenade::vx::network::abstract::CalibratedNeuron const*>(
		            &population->get_cell());
		    !neuron) {
			return false;
		}
	} else {
		return false;
	}
	return true;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
MulticompartmentNeuronMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::LinkedTopology const& mapped_topology) const
{
	std::vector<std::vector<signal_flow::vertex::NeuronView::Parameterization::Config>>
	    mapped_configs(linked_vertex_descriptors.size());

	auto const& mapped_neuron_parameter_space =
	    dynamic_cast<CalibratedNeuron::ParameterSpace const&>(
	        dynamic_cast<grenade::common::Population const&>(
	            mapped_topology.get(linked_vertex_descriptors.at(0)))
	            .get_parameter_space());

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> mapped_parameterizations;

	mapped_parameterizations.push_back({});
	mapped_parameterizations.back().push_back(nullptr);
	mapped_parameterizations.back().emplace_back(
	    std::make_unique<CalibratedNeuron::ParameterSpace::Parameterization>(
	        mapped_neuron_parameter_space.calibration_targets,
	        mapped_neuron_parameter_space.membrane_capacitance, readout_sources));

	return mapped_parameterizations;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> MulticompartmentNeuronMapping::copy() const
{
	return std::make_unique<MulticompartmentNeuronMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> MulticompartmentNeuronMapping::move()
{
	return std::make_unique<MulticompartmentNeuronMapping>(std::move(*this));
}

std::ostream& MulticompartmentNeuronMapping::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "MulticompartmentNeuronMapping(\n";
	ios << hate::Indentation("\t");
	ios << "readout_sources:\n";
	for (size_t j = 0; auto const& nrn : readout_sources) {
		ios << hate::Indentation("\t\t");
		ios << "neuron_on_population " << j << ":\n";
		for (auto const& [compartment_on_neuron, ans] : nrn) {
			ios << hate::Indentation("\t\t\t");
			ios << compartment_on_neuron << ": [" << hate::join(ans.begin(), ans.end(), ", ")
			    << "]\n";
		}
		ios << "\n";
		j++;
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool MulticompartmentNeuronMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return readout_sources ==
	       static_cast<MulticompartmentNeuronMapping const&>(other).readout_sources;
}

} // namespace grenade::vx::network::abstract
