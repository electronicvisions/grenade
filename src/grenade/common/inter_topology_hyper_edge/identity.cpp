#include "grenade/common/inter_topology_hyper_edge/identity.h"
#include "grenade/common/vertex.h"
#include <memory>

namespace grenade::common {

bool IdentityInterTopologyHyperEdge::valid(
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& link_vertex_descriptors,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
    LinkedTopology const&) const
{
	return reference_vertex_descriptors.size() == link_vertex_descriptors.size();
}

std::vector<std::vector<std::unique_ptr<PortData>>> IdentityInterTopologyHyperEdge::map_input_data(
    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&
        reference_input_data,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const&,
    LinkedTopology const&) const
{
	std::vector<std::vector<std::unique_ptr<PortData>>> ret(linked_vertex_descriptors.size());
	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		for (size_t j = 0; j < reference_input_data.at(i).size(); ++j) {
			if (reference_input_data.at(i).at(j)) {
				ret.at(i).push_back(std::unique_ptr<PortData>(static_cast<PortData*>(
				    reference_input_data.at(i).at(j).value().get().copy().release())));
			} else {
				ret.at(i).push_back(nullptr);
			}
		}
	}

	return ret;
}

std::vector<std::vector<std::unique_ptr<PortData>>> IdentityInterTopologyHyperEdge::map_output_data(
    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&
        link_output_data,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const&,
    LinkedTopology const&) const
{
	std::vector<std::vector<std::unique_ptr<PortData>>> ret(linked_vertex_descriptors.size());
	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		for (size_t j = 0; j < link_output_data.at(i).size(); ++j) {
			if (link_output_data.at(i).at(j)) {
				ret.at(i).push_back(std::unique_ptr<PortData>(static_cast<PortData*>(
				    link_output_data.at(i).at(j).value().get().copy().release())));
			} else {
				ret.at(i).push_back(nullptr);
			}
		}
	}

	return ret;
}

std::unique_ptr<InterTopologyHyperEdge> IdentityInterTopologyHyperEdge::copy() const
{
	return std::make_unique<IdentityInterTopologyHyperEdge>(*this);
}

std::unique_ptr<InterTopologyHyperEdge> IdentityInterTopologyHyperEdge::move()
{
	return std::make_unique<IdentityInterTopologyHyperEdge>(std::move(*this));
}

bool IdentityInterTopologyHyperEdge::is_equal_to(InterTopologyHyperEdge const&) const
{
	return true;
}

std::ostream& IdentityInterTopologyHyperEdge::print(std::ostream& os) const
{
	return os << "IdentityInterTopologyHyperEdge()";
}

} // namespace grenade::common
