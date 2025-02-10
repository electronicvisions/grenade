#include "grenade/common/inter_topology_hyper_edge.h"

#include "grenade/common/linked_topology.h"

namespace grenade::common {

InterTopologyHyperEdge::~InterTopologyHyperEdge() {}

bool InterTopologyHyperEdge::valid(
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const&,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const&,
    LinkedTopology const&) const
{
	return true;
}

std::vector<std::vector<std::unique_ptr<PortData>>> InterTopologyHyperEdge::map_input_data(
    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const&,
    LinkedTopology const& topology) const
{
	std::vector<std::vector<std::unique_ptr<PortData>>> ret;
	ret.reserve(linked_vertex_descriptors.size());
	for (auto const& linked_vertex_descriptor : linked_vertex_descriptors) {
		auto const& vertex = topology.get(linked_vertex_descriptor);
		ret.push_back(std::vector<std::unique_ptr<PortData>>(vertex.get_input_ports().size()));
	}
	return ret;
}

std::vector<std::vector<std::unique_ptr<PortData>>> InterTopologyHyperEdge::map_output_data(
    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const&,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
    LinkedTopology const& topology) const
{
	std::vector<std::vector<std::unique_ptr<PortData>>> ret;
	ret.reserve(reference_vertex_descriptors.size());
	for (auto const& reference_vertex_descriptor : reference_vertex_descriptors) {
		auto const& vertex = topology.get_reference().get(reference_vertex_descriptor);
		ret.push_back({});
		ret.back().resize(vertex.get_input_ports().size());
	}
	return ret;
}


std::unique_ptr<InterTopologyHyperEdge> InterTopologyHyperEdge::copy() const
{
	return std::make_unique<InterTopologyHyperEdge>(*this);
}

std::unique_ptr<InterTopologyHyperEdge> InterTopologyHyperEdge::move()
{
	return std::make_unique<InterTopologyHyperEdge>(std::move(*this));
}

bool InterTopologyHyperEdge::is_equal_to(InterTopologyHyperEdge const&) const
{
	return true;
}

std::ostream& InterTopologyHyperEdge::print(std::ostream& os) const
{
	return os << "InterTopologyHyperEdge()";
}

} // namespace grenade::common
