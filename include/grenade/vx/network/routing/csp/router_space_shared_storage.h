#pragma once

#include "grenade/vx/network/routing/csp/route_collector.h"
#include "grenade/vx/network/routing/csp/tracer.h"
#include <log4cxx/logger.h>

namespace grenade::vx::network::routing::csp {

/**
 * Storage shared between routing spaces during branching in CSP search.
 */
struct RouterSpaceSharedStorage
{
	struct SpatialVertexInformation
	{
		using Coordinates = std::vector<size_t>;

		/**
		 * Get coordinates of vertex.
		 * @param vertex Vertex to get coordinates for.
		 */
		Coordinates get_coordinates(grenade::common::VertexOnTopology const& vertex) const;

		/**
		 * Set coordinates of vertex.
		 * @param vertex Vertex to set coordinates for.
		 * @param coordinates Coordinates to be set.
		 */
		void set_coordinates(
		    grenade::common::VertexOnTopology const& vertex, Coordinates const& coordinates);

	private:
		std::map<grenade::common::VertexOnTopology, std::vector<size_t>> m_coordinates;
	} spatial_vertex_information;

	RouterSpaceSharedStorage();

	log4cxx::LoggerPtr logger;

	// Shared ownership across complete lifetime of space and offsprings required due to
	// reference-based internal usage.
	IntTracer int_tracer;
};

} // namespace grenade::vx::network::routing::csp