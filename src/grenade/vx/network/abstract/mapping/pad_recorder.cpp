#include "grenade/vx/network/abstract/mapping/pad_recorder.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/recorder.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/recorder/pad.h"
#include "grenade/vx/signal_flow/vertex/pad_readout.h"

namespace grenade::vx::network::abstract {

PadRecorderMapping::PadRecorderMapping() {}

bool PadRecorderMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	for (auto const& mapped_vertex_descriptor : linked_vertex_descriptors) {
		auto const& mapped_vertex = topology.get(mapped_vertex_descriptor);
		if (dynamic_cast<grenade::vx::signal_flow::vertex::PadReadoutView const*>(&mapped_vertex) ==
		    nullptr) {
			return false;
		}
	}
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	for (auto const& reference_vertex_descriptor : reference_vertex_descriptors) {
		if (auto const recorder = dynamic_cast<grenade::vx::network::abstract::PadRecorder const*>(
		        &topology.get_reference().get(reference_vertex_descriptor));
		    recorder) {
			return recorder->get_shape().size() == linked_vertex_descriptors.size();
		} else {
			return false;
		}
	}
	return true;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
PadRecorderMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        reference_input_data,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::LinkedTopology const&) const
{
	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ret;
	auto const& reference_parameterization = dynamic_cast<PadRecorder::Parameterization const&>(
	    reference_input_data.at(0).at(1).value().get());
	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		ret.push_back({});
		ret.back().emplace_back(nullptr);
		auto linked_parameterization =
		    std::make_unique<signal_flow::vertex::PadReadoutView::Parameterization>();
		linked_parameterization->enable_buffered = reference_parameterization.enable_buffered.at(i);
		ret.back().emplace_back(std::move(linked_parameterization));
	}
	return ret;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> PadRecorderMapping::copy() const
{
	return std::make_unique<PadRecorderMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> PadRecorderMapping::move()
{
	return std::make_unique<PadRecorderMapping>(std::move(*this));
}

std::ostream& PadRecorderMapping::print(std::ostream& os) const
{
	return os << "PadRecorderMapping()";
}

bool PadRecorderMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return InterTopologyHyperEdge::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
