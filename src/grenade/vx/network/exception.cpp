#include "grenade/vx/network/exception.h"

namespace grenade::vx::network {

InvalidNetworkGraph::InvalidNetworkGraph(std::string const& message) : m_message(message) {}

const char* InvalidNetworkGraph::what() const noexcept
{
	return m_message.c_str();
}

} // namespace grenade::vx::network
