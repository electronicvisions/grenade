#include "grenade/common/inter_topology_hyper_edge/fixture.h"

#include "grenade/common/linked_topology.h"
#include "hate/indent.h"


namespace grenade::common {

FixtureInterTopologyHyperEdge::FixtureInterTopologyHyperEdge(PortDataPerPort port_data_per_port) :
    port_data_per_port(std::move(port_data_per_port))
{
}

FixtureInterTopologyHyperEdge::FixtureInterTopologyHyperEdge(
    size_t input_port_on_vertex, PortData const& port_data) :
    port_data_per_port()
{
	port_data_per_port.set(input_port_on_vertex, port_data);
}

FixtureInterTopologyHyperEdge::FixtureInterTopologyHyperEdge(
    size_t input_port_on_vertex, PortData&& port_data) :
    port_data_per_port()
{
	port_data_per_port.set(input_port_on_vertex, std::move(port_data));
}

bool FixtureInterTopologyHyperEdge::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (!reference_vertex_descriptors.empty()) {
		return false;
	}
	for (auto const& linked_vertex_descriptor : linked_vertex_descriptors) {
		auto const num_input_ports =
		    topology.get(linked_vertex_descriptor).get_input_ports().size();
		for (auto const& [input_port_on_vertex, _] : port_data_per_port) {
			if (input_port_on_vertex >= num_input_ports) {
				return false;
			}
		}
	}
	return true;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
FixtureInterTopologyHyperEdge::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::LinkedTopology const& topology) const
{
	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> mapped_port_data;

	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		size_t const size = topology.get(linked_vertex_descriptors.at(i)).get_input_ports().size();
		mapped_port_data.push_back(std::vector<std::unique_ptr<grenade::common::PortData>>(size));
		for (auto const& [input_port_on_vertex, port_data] : port_data_per_port) {
			mapped_port_data.back().at(input_port_on_vertex) = port_data.copy();
		}
	}

	return mapped_port_data;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> FixtureInterTopologyHyperEdge::copy() const
{
	return std::make_unique<FixtureInterTopologyHyperEdge>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> FixtureInterTopologyHyperEdge::move()
{
	return std::make_unique<FixtureInterTopologyHyperEdge>(std::move(*this));
}

std::ostream& FixtureInterTopologyHyperEdge::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "FixtureInterTopologyHyperEdge(\n";
	ios << hate::Indentation("\t");
	for (auto const& [input_port_on_vertex, port_data] : port_data_per_port) {
		ios << input_port_on_vertex << ": " << port_data << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool FixtureInterTopologyHyperEdge::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return port_data_per_port ==
	           static_cast<FixtureInterTopologyHyperEdge const&>(other).port_data_per_port &&
	       InterTopologyHyperEdge::is_equal_to(other);
}

} // namespace grenade::common
