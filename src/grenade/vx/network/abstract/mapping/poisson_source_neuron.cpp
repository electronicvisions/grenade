#include "grenade/vx/network/abstract/mapping/poisson_source_neuron.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/population.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/signal_flow/vertex/background_spike_source.h"
#include "haldls/vx/v3/background.h"
#include "hate/indent.h"

namespace grenade::vx::network::abstract {

PoissonSourceNeuronMapping::PoissonSourceNeuronMapping(
    std::vector<halco::hicann_dls::vx::NeuronLabel> labels,
    std::vector<haldls::vx::v3::BackgroundSpikeSource::Mask> masks) :
    labels(labels), masks(masks)
{
}

bool PoissonSourceNeuronMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	for (auto const& mapped_vertex_descriptor : linked_vertex_descriptors) {
		auto const& mapped_vertex = topology.get(mapped_vertex_descriptor);
		if (dynamic_cast<grenade::vx::signal_flow::vertex::BackgroundSpikeSource const*>(
		        &mapped_vertex) == nullptr) {
			return false;
		}
	}
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const population = dynamic_cast<grenade::common::Population const*>(
	        &topology.get_reference().get(reference_vertex_descriptors.at(0)));
	    population) {
		if (auto const neuron =
		        dynamic_cast<grenade::vx::network::abstract::PoissonSourceNeuron const*>(
		            &population->get_cell());
		    neuron) {
			return true;
		}
	}
	return false;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
PoissonSourceNeuronMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        reference_vertex_input_data,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::LinkedTopology const&) const
{
	auto const& model_vertex_parameterization =
	    dynamic_cast<grenade::vx::network::abstract::PoissonSourceNeuron::ParameterSpace::
	                     Parameterization const&>(
	        reference_vertex_input_data.at(0).at(0).value().get());

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ret;
	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		haldls::vx::v3::BackgroundSpikeSource config;
		config.set_enable(true);
		config.set_enable_random(true);
		config.set_neuron_label(labels.at(i));
		config.set_mask(masks.at(i));
		config.set_rate(model_vertex_parameterization.rate);
		config.set_period(model_vertex_parameterization.period);
		config.set_seed(model_vertex_parameterization.seed);

		grenade::vx::signal_flow::vertex::BackgroundSpikeSource::Parameterization
		    mapped_vertex_parameterization(std::move(config));

		ret.push_back({});
		ret.back().emplace_back(mapped_vertex_parameterization.move());
	}
	return ret;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> PoissonSourceNeuronMapping::copy() const
{
	return std::make_unique<PoissonSourceNeuronMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> PoissonSourceNeuronMapping::move()
{
	return std::make_unique<PoissonSourceNeuronMapping>(std::move(*this));
}

std::ostream& PoissonSourceNeuronMapping::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "PoissonSourceNeuronMapping(\n";
	ios << hate::Indentation("\t");
	ios << "labels:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& label : labels) {
		ios << label << "\n";
	}
	ios << hate::Indentation("\t");
	ios << "masks:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& mask : masks) {
		ios << mask << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool PoissonSourceNeuronMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return InterTopologyHyperEdge::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
