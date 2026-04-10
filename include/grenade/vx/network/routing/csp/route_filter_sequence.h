#pragma once
#include "grenade/common/vertex_on_topology.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <vector>

namespace grenade::vx::network::routing::csp {

/**
 * Sequence of filters applied on a given route for event propagation via vertices in a routing
 * graph.
 */
struct RouteFilterSequence
{
	/**
	 * Vertex descriptor sequence corresponding to sequence of filter vertices.
	 */
	typedef std::vector<grenade::common::VertexOnTopology> Descriptors;
	Descriptors descriptors;

	RouteFilterSequence(Descriptors descriptors);

	bool operator==(RouteFilterSequence const& other) const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, RouteFilterSequence const& value)
	    SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network::routing::csp