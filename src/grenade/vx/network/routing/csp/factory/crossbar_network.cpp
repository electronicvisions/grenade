#include "grenade/vx/network/routing/csp/factory/crossbar_network.h"
#include "grenade/common/edge.h"
#include "grenade/vx/network/routing/csp/vertex/crossbar_node.h"
#include "grenade/vx/network/routing/csp/vertex/crossbar_terminal.h"
#include "grenade/vx/network/routing/csp/vertex/source.h"
#include "grenade/vx/network/routing/csp/vertex/target.h"

namespace grenade::vx::network::routing::csp {

grenade::common::VertexOnTopology CrossbarRouterSpaceFactory::add_source(
    size_t number_label_bits, size_t size)
{
	return get_graph().add_vertex(Source(number_label_bits, size, get_space()));
}

grenade::common::VertexOnTopology CrossbarRouterSpaceFactory::add_target()
{
	return get_graph().add_vertex(Target());
}

void CrossbarRouterSpaceFactory::connect(
    grenade::common::VertexOnTopology const& source,
    grenade::common::VertexOnTopology const& target)
{
	get_graph().add_edge(source, target, grenade::common::Edge());
}

void CrossbarRouterSpaceFactory::connect_to_crossbar(
    grenade::common::VertexOnTopology const& source,
    CrossbarOnRoutingGraph const& crossbar,
    size_t channel)
{
	connect(source, m_crossbars.get(crossbar).inputs.at(channel));
}

void CrossbarRouterSpaceFactory::connect_from_crossbar(
    grenade::common::VertexOnTopology const& target,
    CrossbarOnRoutingGraph const& crossbar,
    size_t channel)
{
	connect(m_crossbars.get(crossbar).outputs.at(channel), target);
}

void CrossbarRouterSpaceFactory::connect_crossbars(
    CrossbarOnRoutingGraph const& source,
    size_t source_channel,
    CrossbarOnRoutingGraph const& target,
    size_t target_channel)
{
	connect(
	    m_crossbars.get(target).outputs.at(target_channel),
	    m_crossbars.get(source).inputs.at(source_channel));
}

CrossbarOnRoutingGraph CrossbarRouterSpaceFactory::add_crossbar(
    size_t x_size, size_t y_size, size_t crossbar_node_bit_count, bool diagonal_elements)
{
	Crossbar crossbar;
	crossbar.inputs.resize(y_size);
	crossbar.outputs.resize(x_size);

	for (size_t y = 0; y < y_size; y++) {
		crossbar.inputs.at(y) = get_graph().add_vertex(CrossbarTerminal());
	}
	for (size_t x = 0; x < x_size; x++) {
		crossbar.outputs.at(x) = get_graph().add_vertex(CrossbarTerminal());
	}

	for (size_t y = 0; y < y_size; y++) {
		for (size_t x = 0; x < x_size; x++) {
			if (x == y && !diagonal_elements) {
				continue;
			}
			auto crossbar_descriptor =
			    get_graph().add_vertex(CrossbarNode(crossbar_node_bit_count, get_space()));
			crossbar.crossbar_nodes[{x, y}] = crossbar_descriptor;
			connect(crossbar.inputs.at(y), crossbar_descriptor);
			connect(crossbar_descriptor, crossbar.outputs.at(x));
		}
	}

	return m_crossbars.insert(crossbar);
}

} // namespace grenade::vx::network::routing::csp