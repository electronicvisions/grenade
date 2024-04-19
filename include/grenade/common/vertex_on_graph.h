#pragma once
#include "grenade/common/genpybind.h"
#include "hate/type_index.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <ostream>
#include <utility>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Identifier of vertex on graph.
 */
template <typename Derived, typename Backend>
struct VertexOnGraph
{
	typedef VertexOnGraph Base;

	typedef typename Backend::vertex_descriptor Value;

	VertexOnGraph() = default;
	VertexOnGraph(Value const& value);

	Value const& value() const GENPYBIND(hidden);

	// needed explicitly for Python wrapping
	bool operator<(Derived const&) const;
	bool operator<=(Derived const&) const;
	bool operator>(Derived const&) const;
	bool operator>=(Derived const&) const;
	bool operator==(Derived const&) const;
	bool operator!=(Derived const&) const;

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

} // namespace common
} // namespace grenade

namespace std {

template <typename T>
struct hash;

template <typename Derived, typename Backend>
struct hash<grenade::common::VertexOnGraph<Derived, Backend>>
{
	size_t operator()(grenade::common::VertexOnGraph<Derived, Backend> const& value) const
	{
		return std::hash<typename grenade::common::VertexOnGraph<Derived, Backend>::Value>{}(
		    value.value());
	}
};

} // namespace std