#pragma once

#include "grenade/vx/network/abstract/detail/graph.h"
#include "grenade/vx/network/abstract/vertex_on_graph.h"

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct CompartmentOnNeuron;
extern template struct SYMBOL_VISIBLE VertexOnGraph<CompartmentOnNeuron, detail::UndirectedGraph>;

struct GENPYBIND(visible) CompartmentOnNeuron
    : public VertexOnGraph<CompartmentOnNeuron, detail::UndirectedGraph>
{
	using VertexOnGraph::VertexOnGraph;
};


} // namespace network
} // namespace grenade::vx

namespace std {

template <>
struct hash<grenade::vx::network::CompartmentOnNeuron>
{
	size_t operator()(grenade::vx::network::CompartmentOnNeuron const& value) const
	{
		return std::hash<typename grenade::vx::network::CompartmentOnNeuron::Base>{}(value);
	}
};

} // namespace std
