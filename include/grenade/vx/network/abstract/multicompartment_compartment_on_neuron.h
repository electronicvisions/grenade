#pragma once
#include "grenade/common/detail/graph.h"
#include "grenade/common/vertex_on_graph.h"
#include "grenade/vx/genpybind.h"

namespace grenade::vx::network::abstract {
struct CompartmentOnNeuron;
} // namespace grenade::vx::network::abstract

namespace grenade::common {
extern template struct SYMBOL_VISIBLE
    VertexOnGraph<vx::network::abstract::CompartmentOnNeuron, detail::UndirectedGraph>;
} // namespace grenade::common

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {


struct GENPYBIND(inline_base("*")) CompartmentOnNeuron
    : public common::VertexOnGraph<CompartmentOnNeuron, common::detail::UndirectedGraph>
{
	using VertexOnGraph::VertexOnGraph;
};


} // namespace abstract
} // namespace grenade::vx::network

namespace std {

template <>
struct hash<grenade::vx::network::abstract::CompartmentOnNeuron>
{
	size_t operator()(grenade::vx::network::abstract::CompartmentOnNeuron const& value) const
	{
		return std::hash<typename grenade::vx::network::abstract::CompartmentOnNeuron::Base>{}(
		    value);
	}
};

} // namespace std
