#include "grenade/vx/network/abstract/mapping/spike_recorder.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/recorder.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/recorder/spike.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"
#include "hate/indent.h"
#include "hate/join.h"


namespace grenade::vx::network::abstract {

SpikeRecorderMapping::SpikeRecorderMapping(std::vector<signal_flow::SpikeFromChip> labels) :
    labels(std::move(labels))
{
}

bool SpikeRecorderMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	for (auto const& mapped_vertex_descriptor : linked_vertex_descriptors) {
		auto const& mapped_vertex = topology.get(mapped_vertex_descriptor);
		if (dynamic_cast<grenade::vx::signal_flow::vertex::CrossbarL2Output const*>(
		        &mapped_vertex) == nullptr) {
			return false;
		}
	}
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const recorder = dynamic_cast<grenade::vx::network::abstract::SpikeRecorder const*>(
	        &dynamic_cast<grenade::common::LinkedTopology const&>(topology.get_reference())
	             .get_reference()
	             .get(reference_vertex_descriptors.at(0)));
	    recorder) {
		return recorder->get_shape().size() == labels.size();
	}
	return false;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
SpikeRecorderMapping::map_output_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        linked_vertex_output_data,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (linked_vertex_output_data.empty()) {
		throw std::invalid_argument("Mapping results not possible without mapped vertex results.");
	}
	size_t const batch_size = dynamic_cast<grenade::common::BatchedPortData const&>(
	                              linked_vertex_output_data.at(0).at(0).value().get())
	                              .batch_size();

	auto const& partitioned_vertex = dynamic_cast<SpikeRecorder const&>(
	    topology.get_reference().get(reference_vertex_descriptors.at(0)));

	std::vector<
	    std::reference_wrapper<grenade::vx::signal_flow::vertex::CrossbarL2Output::Results const>>
	    mapped_spikes_results;
	for (auto const& mapped_vertex_result : linked_vertex_output_data) {
		mapped_spikes_results.emplace_back(std::cref(
		    dynamic_cast<grenade::vx::signal_flow::vertex::CrossbarL2Output::Results const&>(
		        mapped_vertex_result.at(0).value().get())));
	}

	// reverse mapping
	SpikeRecorder::Results::Spikes partitioned_spikes(batch_size);
	for (auto& partitioned_spikes_batch_entry : partitioned_spikes) {
		partitioned_spikes_batch_entry.resize(partitioned_vertex.get_shape().size());
	}

	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		std::unordered_map<signal_flow::SpikeFromChip, size_t> labels_reverse;
		for (size_t j = 0; j < partitioned_vertex.get_shape().size(); ++j) {
			labels_reverse.emplace(labels.at(j), j);
		}

		auto const& mapped_spikes_result = mapped_spikes_results.at(i).get();
		for (size_t b = 0; b < batch_size; ++b) {
			auto& partitioned_spikes_batch_entry = partitioned_spikes.at(b);
			auto const& mapped_spikes_batch_entry = mapped_spikes_result.spikes.at(b);
			for (auto const& mapped_spike : mapped_spikes_batch_entry) {
				if (labels_reverse.contains(mapped_spike.data)) {
					partitioned_spikes_batch_entry.at(labels_reverse.at(mapped_spike.data))
					    .push_back(mapped_spike.time);
				}
			}
		}
	}

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ret;
	ret.push_back({});
	ret.back().emplace_back(
	    std::make_unique<SpikeRecorder::Results>(std::move(partitioned_spikes)));
	return ret;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> SpikeRecorderMapping::copy() const
{
	return std::make_unique<SpikeRecorderMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> SpikeRecorderMapping::move()
{
	return std::make_unique<SpikeRecorderMapping>(std::move(*this));
}

std::ostream& SpikeRecorderMapping::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "SpikeRecorderMapping(\n";
	ios << hate::Indentation("\t");
	ios << hate::join(labels, "\n");
	ios << hate::Indentation() << "\n)";
	return os;
}

bool SpikeRecorderMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return InterTopologyHyperEdge::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
