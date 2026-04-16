#include "grenade/vx/network/routing/csp/router_space_shared_storage.h"

#include <sstream>

namespace grenade::vx::network::routing::csp {

RouterSpaceSharedStorage::RouterSpaceSharedStorage() :
    logger(log4cxx::Logger::getLogger("grenade.network.routing.csp.RouterSpaceSharedStorage"))
{
}

RouterSpaceSharedStorage::SpatialVertexInformation::Coordinates
RouterSpaceSharedStorage::SpatialVertexInformation::get_coordinates(
    grenade::common::VertexOnTopology const& vertex) const
{
	if (!m_coordinates.contains(vertex)) {
		std::stringstream ss;
		ss << "No coordinates for " << vertex << " given.";
		throw std::range_error(ss.str());
	}
	return m_coordinates.at(vertex);
}

void RouterSpaceSharedStorage::SpatialVertexInformation::set_coordinates(
    grenade::common::VertexOnTopology const& vertex, Coordinates const& coordinates)
{
	m_coordinates[vertex] = coordinates;
}

} // namespace grenade::vx::network::routing::csp
