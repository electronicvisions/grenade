#include "grenade/vx/network/abstract/mapping/external_source_neuron.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/population.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"
#include "haldls/vx/v3/event.h"
#include "hate/indent.h"

namespace grenade::vx::network::abstract {

ExternalSourceNeuronMapping::ExternalSourceNeuronMapping(Labels labels) : labels(std::move(labels))
{
}

bool ExternalSourceNeuronMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (linked_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const crossbar_l2_input =
	        dynamic_cast<grenade::vx::signal_flow::vertex::CrossbarL2Input const*>(
	            &topology.get(linked_vertex_descriptors.at(0)));
	    !crossbar_l2_input) {
		return false;
	}
	for (auto const& reference_vertex_descriptor : reference_vertex_descriptors) {
		auto const& reference_vertex = topology.get_reference().get(reference_vertex_descriptor);
		if (auto const population_ptr =
		        dynamic_cast<grenade::common::Population const*>(&reference_vertex);
		    population_ptr) {
			if (auto const neuron_ptr =
			        dynamic_cast<ExternalSourceNeuron const*>(&population_ptr->get_cell());
			    !neuron_ptr) {
				return false;
			}
		} else {
			return false;
		}
	}
	return true;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
ExternalSourceNeuronMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        model_vertex_input_data,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const&) const
{
	if (model_vertex_input_data.empty()) {
		throw std::invalid_argument(
		    "Mapping input_data not possible without model vertex input_data.");
	}
	std::optional<size_t> batch_size;
	for (size_t i = 0; i < reference_vertex_descriptors.size(); ++i) {
		if (model_vertex_input_data.at(i).at(0)) {
			batch_size = dynamic_cast<grenade::common::BatchedPortData const&>(
			                 model_vertex_input_data.at(i).at(0).value().get())
			                 .batch_size();
		}
	}

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ret;
	ret.push_back({});

	if (batch_size) {
		signal_flow::vertex::CrossbarL2Input::Dynamics::Spikes mapped_spikes(*batch_size);

		for (size_t i = 0; i < reference_vertex_descriptors.size(); ++i) {
			if (model_vertex_input_data.at(i).at(0)) {
				auto const& model_vertex_spikes =
				    dynamic_cast<ExternalSourceNeuron::Dynamics const&>(
				        model_vertex_input_data.at(i).at(0).value().get())
				        .spike_times;
				for (auto const& [n, label] : labels.at(i)) {
					for (size_t b = 0; b < batch_size; ++b) {
						for (auto const& time : model_vertex_spikes.at(b).at(n)) {
							for (auto const& l : label) {
								mapped_spikes.at(b).push_back(
								    {time, haldls::vx::v3::SpikePack1ToChip({l})});
							}
						}
					}
				}
			}
		}

		ret.back().emplace_back(std::make_unique<signal_flow::vertex::CrossbarL2Input::Dynamics>(
		    std::move(mapped_spikes)));
	} else {
		ret.back().emplace_back(nullptr);
	}

	return ret;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> ExternalSourceNeuronMapping::copy() const
{
	return std::make_unique<ExternalSourceNeuronMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> ExternalSourceNeuronMapping::move()
{
	return std::make_unique<ExternalSourceNeuronMapping>(std::move(*this));
}

std::ostream& ExternalSourceNeuronMapping::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "ExternalSourceNeuronMapping(\n";
	for (size_t i = 0; i < labels.size(); ++i) {
		ios << hate::Indentation("\t");
		ios << "linked_vertex_index " << i << ":\n";
		ios << hate::Indentation("\t\t");
		for (auto const& [n, local_labels] : labels.at(i)) {
			ios << n << ": [" << hate::join(local_labels, ", ") << "]\n";
		}
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool ExternalSourceNeuronMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return InterTopologyHyperEdge::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
