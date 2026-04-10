#include "grenade/vx/network/routing/csp/router_space.h"

#include "grenade/vx/network/routing/csp/propagator/bitwise_and.h"
#include "grenade/vx/network/routing/csp/route_collector.h"
#include "grenade/vx/network/routing/csp/tracer.h"
#include "grenade/vx/network/routing/csp/vertex/filter.h"
#include "grenade/vx/network/routing/csp/vertex/source.h"
#include "hate/variant.h"
#include <ostream>
#include <stack>
#include <log4cxx/logger.h>

namespace grenade::vx::network::routing::csp {

RouterSpace::RouterSpace() : m_shared_storage(std::make_shared<RouterSpaceSharedStorage>()) {}

RouterSpace::RouterSpace(RouterSpace& other) :
    Gecode::Space(other),
    m_shared_storage(other.m_shared_storage),
    m_routing_graph(other.m_routing_graph)
{
	m_routing_graph.update(*this, other.m_routing_graph);

	for (auto& [source_target_pair, route_filter_sequences] : other.m_route_activities) {
		std::vector<Gecode::BoolVarArray> route_filter_sequences_copy(
		    route_filter_sequences.size());

		for (size_t i = 0; i < route_filter_sequences.size(); ++i) {
			route_filter_sequences_copy.at(i).update(*this, route_filter_sequences.at(i));
		}

		m_route_activities.emplace(source_target_pair, std::move(route_filter_sequences_copy));
	}
}

Gecode::Space* RouterSpace::copy()
{
	return new RouterSpace(*this);
}

RoutingGraph const& RouterSpace::get_graph() const
{
	return m_routing_graph;
}

std::ostream& operator<<(std::ostream& os, RouterSpace const& router_space)
{
	os << "RouterSpace(\n";
	os << router_space.m_routing_graph << "\n";
	os << ")";
	return os;
}

} // namespace grenade::vx::network::routing::csp
