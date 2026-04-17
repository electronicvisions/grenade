#include "grenade/vx/network/abstract/mapping/madc_recorder.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/recorder.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/recorder/madc.h"
#include "grenade/vx/signal_flow/vertex/madc_readout.h"

namespace grenade::vx::network::abstract {

MADCRecorderMapping::MADCRecorderMapping() {}

bool MADCRecorderMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (linked_vertex_descriptors.size() != 1) {
		return false;
	}
	for (auto const& mapped_vertex_descriptor : linked_vertex_descriptors) {
		auto const& mapped_vertex = topology.get(mapped_vertex_descriptor);
		if (dynamic_cast<grenade::vx::signal_flow::vertex::MADCReadoutView const*>(
		        &mapped_vertex) == nullptr) {
			return false;
		}
	}
	if (reference_vertex_descriptors.size() > 2 || reference_vertex_descriptors.empty()) {
		return false;
	}
	for (auto const& reference_vertex_descriptor : reference_vertex_descriptors) {
		if (auto const recorder = dynamic_cast<grenade::vx::network::abstract::MADCRecorder const*>(
		        &topology.get_reference().get(reference_vertex_descriptor));
		    !recorder) {
			return false;
		}
	}
	return true;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
MADCRecorderMapping::map_output_data(
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

	std::vector<
	    std::reference_wrapper<grenade::vx::signal_flow::vertex::MADCReadoutView::Results const>>
	    mapped_madc_results;
	for (auto const& mapped_vertex_result : linked_vertex_output_data) {
		mapped_madc_results.emplace_back(std::cref(
		    dynamic_cast<grenade::vx::signal_flow::vertex::MADCReadoutView::Results const&>(
		        mapped_vertex_result.at(0).value().get())));
	}

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ret;
	size_t channel_offset = 0;
	for (auto const& reference_vertex_descriptor : reference_vertex_descriptors) {
		auto const& partitioned_vertex = dynamic_cast<MADCRecorder const&>(
		    topology.get_reference().get(reference_vertex_descriptor));

		MADCRecorder::Results::Samples partitioned_samples(batch_size);
		for (auto& batch_entry : partitioned_samples) {
			batch_entry.resize(partitioned_vertex.get_shape().size());
		}

		for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
			auto const& mapped_madc_result = mapped_madc_results.at(i).get();
			for (size_t b = 0; b < batch_size; ++b) {
				auto& partitioned_samples_batch_entry = partitioned_samples.at(b);
				auto const& mapped_samples_batch_entry = mapped_madc_result.samples.at(b);
				for (auto const& mapped_sample : mapped_samples_batch_entry) {
					if (channel_offset <= mapped_sample.data.channel.value() &&
					    mapped_sample.data.channel.value() <
					        channel_offset + partitioned_vertex.get_shape().size()) {
						partitioned_samples_batch_entry
						    .at(mapped_sample.data.channel.value() - channel_offset)
						    .push_back(
						        std::make_pair(mapped_sample.time, mapped_sample.data.value));
					}
				}
			}
		}

		ret.push_back({});
		ret.back().emplace_back(
		    std::make_unique<MADCRecorder::Results>(std::move(partitioned_samples)));

		channel_offset += partitioned_vertex.get_shape().size();
	}
	return ret;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
MADCRecorderMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::LinkedTopology const& topology) const
{
	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ret;
	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		auto parameterization =
		    std::make_unique<signal_flow::vertex::MADCReadoutView::Parameterization>();
		if (dynamic_cast<signal_flow::vertex::MADCReadoutView const&>(
		        topology.get(linked_vertex_descriptors.at(i)))
		        .get_second_source()) {
			parameterization->period = haldls::vx::v3::MADCConfig::ActiveMuxInputSelectLength(1);
		}
		ret.push_back({});
		ret.back().emplace_back(nullptr);
		ret.back().emplace_back(std::move(parameterization));
	}
	return ret;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> MADCRecorderMapping::copy() const
{
	return std::make_unique<MADCRecorderMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> MADCRecorderMapping::move()
{
	return std::make_unique<MADCRecorderMapping>(std::move(*this));
}

std::ostream& MADCRecorderMapping::print(std::ostream& os) const
{
	return os << "MADCRecorderMapping()";
}

bool MADCRecorderMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return InterTopologyHyperEdge::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
