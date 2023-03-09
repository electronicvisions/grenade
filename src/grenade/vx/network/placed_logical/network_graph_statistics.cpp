#include "grenade/vx/network/placed_logical/network_graph_statistics.h"

#include "grenade/vx/network/placed_logical/network_graph.h"

namespace grenade::vx::network::placed_logical {

NetworkGraphStatistics extract_statistics(NetworkGraph const& network_graph)
{
	return placed_atomic::extract_statistics(network_graph.get_hardware_network_graph());
}

} // namespace grenade::vx::network::placed_logical
