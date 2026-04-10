#pragma once
#include "grenade/common/vertex_on_topology.h"
#include <map>

namespace grenade::vx::network::routing::csp {

/**
 * Crossbar.
 *
 * Contains all the IO-connectors and all the crossbar nodes.
 */
struct Crossbar
{
	std::vector<grenade::common::VertexOnTopology> inputs;
	std::vector<grenade::common::VertexOnTopology> outputs;

	/**
	 * Crossbar nodes by (x,y) coordinate.
	 */
	std::map<std::pair<size_t, size_t>, grenade::common::VertexOnTopology> crossbar_nodes;
};

} // namespace grenade::vx::network::routing::csp
