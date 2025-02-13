#pragma once
#include "grenade/common/detail/graph.h"
#include "grenade/common/vertex_on_graph.h"
#include "grenade/vx/genpybind.h"

namespace grenade::vx::network {
struct CompartmentOnNeuron;
} // namespace grenade::vx::network

namespace grenade::common {
extern template struct SYMBOL_VISIBLE
    VertexOnGraph<vx::network::CompartmentOnNeuron, detail::UndirectedGraph>;
} // namespace grenade::common

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) CompartmentOnNeuron
    : public common::VertexOnGraph<CompartmentOnNeuron, common::detail::UndirectedGraph>
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
