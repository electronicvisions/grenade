#include "grenade/common/inter_topology_hyper_edge/recorder.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/recorder.h"
#include <stdexcept>

namespace grenade::common {

bool RecorderInterTopologyHyperEdge::valid(
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& link_vertex_descriptors,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
    LinkedTopology const& linked_topology) const
{
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (link_vertex_descriptors.size() == 0) {
		return false;
	}
	if (auto const reference_recorder = dynamic_cast<Recorder const*>(
	        &linked_topology.get_reference().get(reference_vertex_descriptors.at(0)));
	    reference_recorder) {
		for (auto const& link_vertex_descriptor : link_vertex_descriptors) {
			if (typeid(linked_topology.get(link_vertex_descriptor)) !=
			    typeid(*reference_recorder)) {
				return false;
			}
			auto const& linked_recorder =
			    dynamic_cast<Recorder const&>(linked_topology.get(link_vertex_descriptor));
			if (!reference_recorder->get_shape().includes(linked_recorder.get_shape())) {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::vector<std::vector<std::unique_ptr<PortData>>> RecorderInterTopologyHyperEdge::map_input_data(
    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&
        reference_input_data,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const&,
    LinkedTopology const&) const
{
	std::vector<std::vector<std::unique_ptr<PortData>>> ret(linked_vertex_descriptors.size());
	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		for (size_t j = 0; j < reference_input_data.at(0).size(); ++j) {
			if (reference_input_data.at(0).at(j)) {
				ret.at(i).push_back(std::unique_ptr<PortData>(static_cast<PortData*>(
				    reference_input_data.at(0).at(j).value().get().copy().release())));
			} else {
				ret.at(i).push_back(nullptr);
			}
		}
	}

	return ret;
}

std::vector<std::vector<std::unique_ptr<PortData>>> RecorderInterTopologyHyperEdge::map_output_data(
    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&
        link_output_data,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& link_vertex_descriptors,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
    LinkedTopology const& topology) const
{
	auto const& reference_recorder = dynamic_cast<Recorder const&>(
	    topology.get_reference().get(reference_vertex_descriptors.at(0)));

	// if no ports are present return empty output data
	if (link_output_data.at(0).size() == 0) {
		std::vector<std::vector<std::unique_ptr<PortData>>> ret;
		ret.push_back({});
		return ret;
	}

	// use the first linked vertex data to get info
	assert(link_output_data.at(0).size() == 1);
	auto const first_linked_results =
	    dynamic_cast<Recorder::Results const*>(&link_output_data.at(0).at(0).value().get());
	if (!first_linked_results) {
		throw std::invalid_argument("Linked output data is not recorder results.");
	}
	size_t const batch_size = first_linked_results->batch_size();

	std::vector<std::vector<std::unique_ptr<PortData>>> ret;
	ret.push_back({});

	// use the first linked vertex data to get info
	bool const port_has_data = static_cast<bool>(link_output_data.at(0).at(0));
	if (port_has_data) {
		auto reference_output_data = reference_recorder.create_empty_results(batch_size);
		if (!reference_output_data) {
			throw std::runtime_error("Recorder didn't create empty results.");
		}
		for (size_t i = 0; i < link_vertex_descriptors.size(); ++i) {
			auto const& link_recorder =
			    dynamic_cast<Recorder const&>(topology.get(link_vertex_descriptors.at(i)));
			auto const linked_results =
			    dynamic_cast<Recorder::Results const*>(&link_output_data.at(i).at(0).value().get());
			if (!linked_results) {
				throw std::invalid_argument("Linked output data is not recorder results.");
			}
			reference_output_data->set_section(
			    *linked_results,
			    *CuboidMultiIndexSequence({reference_recorder.get_shape().size()})
			         .related_sequence_subset_restriction(
			             reference_recorder.get_shape(), link_recorder.get_shape()));
		}
		ret.back().push_back(std::move(reference_output_data));
	} else {
		ret.back().push_back(nullptr);
	}

	return ret;
}

std::unique_ptr<InterTopologyHyperEdge> RecorderInterTopologyHyperEdge::copy() const
{
	return std::make_unique<RecorderInterTopologyHyperEdge>(*this);
}

std::unique_ptr<InterTopologyHyperEdge> RecorderInterTopologyHyperEdge::move()
{
	return std::make_unique<RecorderInterTopologyHyperEdge>(std::move(*this));
}

bool RecorderInterTopologyHyperEdge::is_equal_to(InterTopologyHyperEdge const&) const
{
	return true;
}

std::ostream& RecorderInterTopologyHyperEdge::print(std::ostream& os) const
{
	return os << "RecorderInterTopologyHyperEdge()";
}

} // namespace grenade::common
