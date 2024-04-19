#pragma once
#include "grenade/common/detail/graph.h"
#include "grenade/common/edge_on_graph.h"
#include "grenade/vx/genpybind.h"

namespace grenade::vx::network::abstract {
struct CompartmentConnectionOnNeuron;
} // namespace grenade::vx::network::abstract

namespace grenade::common {

extern template struct SYMBOL_VISIBLE
    EdgeOnGraph<vx::network::abstract::CompartmentConnectionOnNeuron, detail::UndirectedGraph>;

} // namespace grenade::common

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(inline_base("*")) CompartmentConnectionOnNeuron
    : public common::EdgeOnGraph<CompartmentConnectionOnNeuron, common::detail::UndirectedGraph>
{
	using EdgeOnGraph::EdgeOnGraph;
};

} // namespace abstract
} // namespace grenade::vx::network

namespace std {

template <>
struct hash<grenade::vx::network::abstract::CompartmentConnectionOnNeuron>
{
	size_t operator()(
	    grenade::vx::network::abstract::CompartmentConnectionOnNeuron const& value) const
	{
		return std::hash<
		    typename grenade::vx::network::abstract::CompartmentConnectionOnNeuron::Base>{}(value);
	}
};

} // namespace std
