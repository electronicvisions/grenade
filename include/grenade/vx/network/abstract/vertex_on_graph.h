#pragma once
#include "grenade/vx/genpybind.h"
#include "hate/type_index.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <ostream>
#include <utility>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Identifier of vertex on graph.
 */
template <typename Derived, typename Backend>
struct GENPYBIND(visible) VertexOnGraph
{
	typedef VertexOnGraph Base;

	typedef typename Backend::vertex_descriptor Value;

	VertexOnGraph() = default;
	VertexOnGraph(Value const& value);

	Value const& value() const GENPYBIND(hidden);

	auto operator<=>(VertexOnGraph const& other) const = default;

	GENPYBIND(expose_as(__hash__))
	size_t hash() const;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, VertexOnGraph const& value)
	{
		return value.print(os);
	}

private:
	std::ostream& print(std::ostream& os) const;

	Value m_value;
};


template <typename Derived, typename Backend>
std::ostream& GENPYBIND(stringstream) operator<<(
    std::ostream& os, VertexOnGraph<Derived, Backend> const& value);

} // namespace network
} // namespace grenade::vx

namespace std {

template <typename T>
struct hash;

template <typename Derived, typename Backend>
struct hash<grenade::vx::network::VertexOnGraph<Derived, Backend>>
{
	size_t operator()(grenade::vx::network::VertexOnGraph<Derived, Backend> const& value) const
	{
		return std::hash<typename grenade::vx::network::VertexOnGraph<Derived, Backend>::Value>{}(
		    value.value());
	}
};

} // namespace std
