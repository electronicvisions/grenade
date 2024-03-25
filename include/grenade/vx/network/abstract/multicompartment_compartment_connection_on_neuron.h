#pragma once

#include "grenade/vx/network/abstract/detail/graph.h"
#include "grenade/vx/network/abstract/edge_on_graph.h"

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct CompartmentConnectionOnNeuron;
extern template struct SYMBOL_VISIBLE
    EdgeOnGraph<CompartmentConnectionOnNeuron, detail::UndirectedGraph>;

struct GENPYBIND(visible) CompartmentConnectionOnNeuron
    : public EdgeOnGraph<CompartmentConnectionOnNeuron, detail::UndirectedGraph>
{
	using EdgeOnGraph::EdgeOnGraph;
};

} // namespace grenade::vx::network

namespace std {

template <>
struct hash<grenade::vx::network::CompartmentConnectionOnNeuron>
{
	size_t operator()(grenade::vx::network::CompartmentConnectionOnNeuron const& value) const
	{
		return std::hash<typename grenade::vx::network::CompartmentConnectionOnNeuron::Base>{}(
		    value);
	}
};

} // namespace std