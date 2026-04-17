#include "grenade/vx/network/abstract/mapping/locally_placed_neuron.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/population.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/indent.h"


namespace grenade::vx::network::abstract {

LocallyPlacedNeuronMapping::LocallyPlacedNeuronMapping(Labels labels, Anchors anchors) :
    labels(std::move(labels)), anchors(std::move(anchors))
{
}

bool LocallyPlacedNeuronMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (std::set{anchors.size(), labels.size(), linked_vertex_descriptors.size()}.size() != 1) {
		return false;
	}
	for (auto const& mapped_vertex_descriptor : linked_vertex_descriptors) {
		auto const& mapped_vertex = topology.get(mapped_vertex_descriptor);
		if (dynamic_cast<grenade::vx::signal_flow::vertex::NeuronView const*>(&mapped_vertex) ==
		    nullptr) {
			return false;
		}
	}
	for (auto const& reference_vertex_descriptor : reference_vertex_descriptors) {
		auto const& reference_vertex = topology.get_reference().get(reference_vertex_descriptor);
		if (auto const population_ptr =
		        dynamic_cast<grenade::common::Population const*>(&reference_vertex);
		    population_ptr) {
			if (auto const neuron_ptr =
			        dynamic_cast<LocallyPlacedNeuron const*>(&population_ptr->get_cell());
			    !neuron_ptr) {
				return false;
			}
		} else {
			return false;
		}
	}
	return true;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> LocallyPlacedNeuronMapping::copy() const
{
	return std::make_unique<LocallyPlacedNeuronMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> LocallyPlacedNeuronMapping::move()
{
	return std::make_unique<LocallyPlacedNeuronMapping>(std::move(*this));
}

std::ostream& LocallyPlacedNeuronMapping::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "LocallyPlacedNeuronMapping(\n";
	ios << hate::Indentation("\t");
	ios << "labels:\n";
	ios << hate::Indentation("\t\t");
	for (size_t linked_vertex_index = 0; auto const& local_labels : labels) {
		ios << "linked_vertex_index " << linked_vertex_index << ":\n";
		ios << hate::Indentation("\t\t\t");
		for (auto const& label : local_labels) {
			if (label) {
				ios << *label << "\n";
			} else {
				ios << "nullopt\n";
			}
		}
		linked_vertex_index++;
	}
	ios << hate::Indentation("\t");
	ios << "anchors:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& [cell_on_population, a] : anchors) {
		auto const& [linked_indices, anchor] = a;
		ios << "cell_on_population " << cell_on_population << " (";
		std::vector<std::string> linked_indices_str;
		for (auto const& [hemisphere, linked_index] : linked_indices) {
			std::stringstream ss;
			ss << hemisphere << ": " << linked_index;
			linked_indices_str.push_back(ss.str());
		}
		ios << hate::join(linked_indices_str, ", ") << "): " << anchor << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool LocallyPlacedNeuronMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return InterTopologyHyperEdge::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
