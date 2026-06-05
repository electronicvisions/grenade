#include "grenade/common/topology_lazy_validity_checker.h"

#include "grenade/common/topology.h"

namespace grenade::common {

TopologyLazyValidityChecker::TopologyLazyValidityChecker(Topology& topology) : m_topology(topology)
{
	if (m_topology.m_has_lazy_validity_checker) {
		throw std::invalid_argument("Topology already has an active lazy validity checker.");
	}
	m_topology.m_has_lazy_validity_checker = true;
}

TopologyLazyValidityChecker::~TopologyLazyValidityChecker()
{
	// check all edges for all vertices
	for (auto const& descriptor : m_topology.vertices()) {
		for (auto const& in_edge_descriptor : m_topology.in_edges(descriptor)) {
			auto const& source_vertex_descriptor = m_topology.source(in_edge_descriptor);
			m_topology.check_edge(
			    source_vertex_descriptor, descriptor, in_edge_descriptor,
			    m_topology.get(source_vertex_descriptor), m_topology.get(descriptor),
			    m_topology.get(in_edge_descriptor));
		}
		for (auto const& out_edge_descriptor : m_topology.out_edges(descriptor)) {
			auto const& target_vertex_descriptor = m_topology.target(out_edge_descriptor);
			m_topology.check_edge(
			    descriptor, target_vertex_descriptor, out_edge_descriptor,
			    m_topology.get(descriptor), m_topology.get(target_vertex_descriptor),
			    m_topology.get(out_edge_descriptor));
		}
	}
	m_topology.m_has_lazy_validity_checker = false;
}

} // namespace grenade::common
