#include "grenade/common/topology.h"

#include "grenade/common/edge_on_topology.h"
#include "grenade/common/graph_impl.tcc"
#include "grenade/common/vertex_on_topology.h"
#include "hate/join.h"
#include <log4cxx/logger.h>

namespace grenade::common {

template struct Graph<
    Topology,
    detail::BidirectionalMultiGraph,
    Vertex,
    Edge,
    VertexOnTopology,
    EdgeOnTopology,
    std::shared_ptr>;

template std::ostream& operator<<(
    std::ostream& os,
    Graph<
        Topology,
        detail::BidirectionalMultiGraph,
        Vertex,
        Edge,
        VertexOnTopology,
        EdgeOnTopology,
        std::shared_ptr> const& value);

EdgeOnTopology Topology::add_edge(
    VertexOnTopology const& source, VertexOnTopology const& target, Edge const& edge)
{
	check_edge(source, target, std::nullopt, get(source), get(target), edge);
	return Graph::add_edge(source, target, edge);
}

EdgeOnTopology Topology::add_edge(
    VertexOnTopology const& source, VertexOnTopology const& target, Edge&& edge)
{
	check_edge(source, target, std::nullopt, get(source), get(target), edge);
	return Graph::add_edge(source, target, std::move(edge));
}

void Topology::set(VertexDescriptor const& descriptor, Vertex const& vertex)
{
	for (auto const& in_edge_descriptor : in_edges(descriptor)) {
		auto const& source_vertex_descriptor = source(in_edge_descriptor);
		check_edge(
		    source_vertex_descriptor, descriptor, in_edge_descriptor, get(source_vertex_descriptor),
		    vertex, get(in_edge_descriptor));
	}
	for (auto const& out_edge_descriptor : out_edges(descriptor)) {
		auto const& target_vertex_descriptor = target(out_edge_descriptor);
		check_edge(
		    descriptor, target_vertex_descriptor, out_edge_descriptor, vertex,
		    get(target_vertex_descriptor), get(out_edge_descriptor));
	}
	Graph::set(descriptor, vertex);
}

void Topology::set(VertexDescriptor const& descriptor, Vertex&& vertex)
{
	for (auto const& in_edge_descriptor : in_edges(descriptor)) {
		auto const& source_vertex_descriptor = source(in_edge_descriptor);
		check_edge(
		    source_vertex_descriptor, descriptor, in_edge_descriptor, get(source_vertex_descriptor),
		    vertex, get(in_edge_descriptor));
	}
	for (auto const& out_edge_descriptor : out_edges(descriptor)) {
		auto const& target_vertex_descriptor = target(out_edge_descriptor);
		check_edge(
		    descriptor, target_vertex_descriptor, out_edge_descriptor, vertex,
		    get(target_vertex_descriptor), get(out_edge_descriptor));
	}
	Graph::set(descriptor, vertex);
}

void Topology::set(EdgeDescriptor const& descriptor, Edge const& edge)
{
	auto const& source_vertex_descriptor = source(descriptor);
	auto const& target_vertex_descriptor = target(descriptor);
	check_edge(
	    source_vertex_descriptor, target_vertex_descriptor, descriptor,
	    get(source_vertex_descriptor), get(target_vertex_descriptor), edge);
	Graph::set(descriptor, edge);
}

void Topology::set(EdgeDescriptor const& descriptor, Edge&& edge)
{
	auto const& source_vertex_descriptor = source(descriptor);
	auto const& target_vertex_descriptor = target(descriptor);
	check_edge(
	    source_vertex_descriptor, target_vertex_descriptor, descriptor,
	    get(source_vertex_descriptor), get(target_vertex_descriptor), edge);
	Graph::set(descriptor, edge);
}

void Topology::check_edge(
    VertexOnTopology const& source,
    VertexOnTopology const& target,
    std::optional<EdgeOnTopology> const& edge_descriptor,
    Vertex const& source_vertex,
    Vertex const& target_vertex,
    Edge const& edge) const
{
	if (!source_vertex.valid_edge_to(target_vertex, edge)) {
		std::stringstream ss;
		ss << "Edge from source (" << source << ") to target (" << target
		   << ") is not valid for the source vertex to the target vertex:\n";
		ss << source_vertex << "\n";
		ss << target_vertex << "\n";
		ss << edge << "\n";
		throw std::invalid_argument(ss.str());
	}
	if (!target_vertex.valid_edge_from(source_vertex, edge)) {
		std::stringstream ss;
		ss << "Edge from source (" << source << ") to target (" << target
		   << ") is not valid for the target vertex from the source vertex:\n";
		ss << source_vertex << "\n";
		ss << target_vertex << "\n";
		ss << edge << "\n";
		throw std::invalid_argument(ss.str());
	}
	auto const target_vertex_ports = target_vertex.get_input_ports();
	auto const& target_vertex_port = target_vertex_ports.at(edge.port_on_target);
	if (!static_cast<bool>(target_vertex_port.sum_or_split_support)) {
		std::set<size_t> usage;
		for (auto const& in_edge_descriptor : in_edges(target)) {
			if (edge_descriptor && in_edge_descriptor == *edge_descriptor) {
				continue;
			}
			auto const& in_edge = get(in_edge_descriptor);
			if (in_edge.port_on_target != edge.port_on_target) {
				continue;
			}
			if (!in_edge.get_channels_on_target().is_disjunct(edge.get_channels_on_target())) {
				std::stringstream ss;
				ss << "Edge from source (" << source << ") to target (" << target
				   << ") is not valid, because there are multiple usages of channels on the target "
				      "vertex port, "
				      "which doesn't support it:\n";
				ss << "source vertex: " << source_vertex << "\n";
				ss << "target vertex: " << target_vertex << "\n";
				ss << "edge: " << edge << "\n";
				ss << "other edge: " << in_edge << "\n";
				throw std::invalid_argument(ss.str());
			}
		}
	}
	auto const source_vertex_ports = source_vertex.get_output_ports();
	auto const& source_vertex_port = source_vertex_ports.at(edge.port_on_source);
	if (!static_cast<bool>(source_vertex_port.sum_or_split_support)) {
		std::set<size_t> usage;
		for (auto const& out_edge_descriptor : out_edges(source)) {
			if (edge_descriptor && out_edge_descriptor == *edge_descriptor) {
				continue;
			}
			auto const& out_edge = get(out_edge_descriptor);
			if (out_edge.port_on_source != edge.port_on_source) {
				continue;
			}
			if (!out_edge.get_channels_on_source().is_disjunct(edge.get_channels_on_source())) {
				std::stringstream ss;
				ss << "Edge from source (" << source << ") to target (" << target
				   << ") is not valid, because there are multiple usages of channels on the source "
				      "vertex "
				      "port, "
				      "which doesn't support it.";
				throw std::invalid_argument(ss.str());
			}
		}
	}
}

bool Topology::valid_strong_components() const
{
	std::map<size_t, std::set<VertexOnTopology>> strong_components_map;
	{
		auto const strong_components_inv = strong_components();
		for (auto const& [vertex_descriptor, color] : strong_components_inv) {
			strong_components_map[color].insert(vertex_descriptor);
		}
	}
	for (auto const& [_, strong_component] : strong_components_map) {
		if (strong_component.size() == 1) {
			continue;
		}
		std::unique_ptr<Vertex::StrongComponentInvariant> last_invariant;
		for (auto const& vertex_descriptor : strong_component) {
			auto invariant = get(vertex_descriptor).get_strong_component_invariant();
			assert(invariant);
			if (last_invariant && (*last_invariant != *invariant)) {
				LOG4CXX_ERROR(
				    log4cxx::Logger::getLogger("grenade.common.Topology"),
				    "valid_strong_components(): "
				        << "Invariants not equal in strong component:\n"
				        << hate::join(
				               strong_component, "\n",
				               [&](auto const& vertex_descriptor) {
					               std::stringstream ss;
					               ss << vertex_descriptor << ": ";
					               auto const invariant =
					                   get(vertex_descriptor).get_strong_component_invariant();
					               assert(invariant);
					               ss << *invariant;
					               return ss.str();
				               })
				        << ".");
				return false;
			}
			last_invariant = std::move(invariant);
		}
	}
	return true;
}

bool Topology::valid() const
{
	return valid_strong_components();
}

bool Topology::operator==(Topology const& other) const
{
	return Graph::operator==(other);
}

bool Topology::operator!=(Topology const& other) const
{
	return Graph::operator!=(other);
}

std::ostream& Topology::print(std::ostream& os) const
{
	return os << static_cast<Topology::BaseGraph const&>(*this);
}

} // namespace grenade::common
