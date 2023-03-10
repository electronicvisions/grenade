#include "grenade/vx/network/placed_logical/exception.h"

namespace grenade::vx::network::placed_logical {

InvalidNetworkGraph::InvalidNetworkGraph(std::string const& message) : m_message(message) {}

const char* InvalidNetworkGraph::what() const noexcept
{
	return m_message.c_str();
}


UnsuccessfulRouting::UnsuccessfulRouting(std::string const& message) : m_message(message) {}

const char* UnsuccessfulRouting::what() const noexcept
{
	return m_message.c_str();
}

} // namespace grenade::vx::network::placed_logical
