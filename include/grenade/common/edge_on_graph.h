#pragma once
#include "grenade/common/genpybind.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <utility>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Identifier of edge on graph.
 */
template <typename Derived, typename Backend>
struct EdgeOnGraph
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

} // namespace common
} // namespace grenade

namespace std {

template <typename Derived, typename Backend>
struct hash<grenade::common::EdgeOnGraph<Derived, Backend>>
{
	size_t operator()(grenade::common::EdgeOnGraph<Derived, Backend> const& value) const
	{
		return std::hash<typename grenade::common::EdgeOnGraph<Derived, Backend>::Value>{}(
		    value.value());
	}
};

} // namespace std
