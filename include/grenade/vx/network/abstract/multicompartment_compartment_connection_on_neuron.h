#pragma once
#include "grenade/common/detail/graph.h"
#include "grenade/common/edge_on_graph.h"
#include "grenade/vx/genpybind.h"

namespace grenade::vx::network {
struct CompartmentConnectionOnNeuron;
} // namespace grenade::vx::network

namespace grenade::common {

extern template struct SYMBOL_VISIBLE
    EdgeOnGraph<vx::network::CompartmentConnectionOnNeuron, detail::UndirectedGraph>;

} // namespace grenade::common

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) CompartmentConnectionOnNeuron
    : public common::EdgeOnGraph<CompartmentConnectionOnNeuron, common::detail::UndirectedGraph>
{
	using EdgeOnGraph::EdgeOnGraph;
};

} // namespace network
} // namespace grenade::vx

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
