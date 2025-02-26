#pragma once
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <utility>

namespace grenade::vx { namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Identifier of edge on graph.
 */
template <typename Derived, typename Backend>
struct GENPYBIND(visible) EdgeOnGraph
{
	typedef EdgeOnGraph Base;

	typedef typename Backend::edge_descriptor Value;

	EdgeOnGraph() = default;
	EdgeOnGraph(Value const& value);

	Value const& value() const GENPYBIND(hidden);

	bool operator==(EdgeOnGraph const& other) const = default;
	bool operator!=(EdgeOnGraph const& other) const = default;

	friend std::ostream& operator<<(std::ostream& os, EdgeOnGraph const& value)
	{
		return value.print(os);
	}

private:
	std::ostream& print(std::ostream& os) const;

	Value m_value;
};

} // namespace network
} // namespace grenade::vx

namespace std {

template <typename T>
struct hash;

template <typename Derived, typename Backend>
struct hash<grenade::vx::network::EdgeOnGraph<Derived, Backend>>
{
	size_t operator()(grenade::vx::network::EdgeOnGraph<Derived, Backend> const& value) const
	{
		return std::hash<typename grenade::vx::network::EdgeOnGraph<Derived, Backend>::Value>{}(
		    value.value());
	}
};

} // namespace std
