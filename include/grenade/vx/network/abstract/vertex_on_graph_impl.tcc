#include "grenade/vx/network/abstract/vertex_on_graph.h"

#include "hate/type_index.h"
#include <ostream>

namespace grenade::vx::network {

template <typename Derived, typename Backend>
VertexOnGraph<Derived, Backend>::VertexOnGraph(Value const& value) : m_value(value)
{}

template <typename Derived, typename Backend>
typename VertexOnGraph<Derived, Backend>::Value const& VertexOnGraph<Derived, Backend>::value()
    const
{
	return m_value;
}

template <typename Derived, typename Backend>
std::ostream& VertexOnGraph<Derived, Backend>::print(std::ostream& os) const
{
	return (os << hate::name<Derived>() << "(" << m_value << ")");
}

} // namespace grenade::vx::network
