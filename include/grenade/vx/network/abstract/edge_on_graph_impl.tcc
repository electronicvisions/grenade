#include "grenade/vx/network/abstract/edge_on_graph.h"

#include "hate/type_index.h"
#include <ostream>

namespace grenade::vx::network {

template <typename Derived, typename Backend>
EdgeOnGraph<Derived, Backend>::EdgeOnGraph(Value const& value) : m_value(value)
{}

template <typename Derived, typename Backend>
typename EdgeOnGraph<Derived, Backend>::Value const& EdgeOnGraph<Derived, Backend>::value() const
{
	return m_value;
}

template <typename Derived, typename Backend>
std::ostream& EdgeOnGraph<Derived, Backend>::print(std::ostream& os) const
{
	return (os << hate::name<Derived>() << "(" << value() << ")");
}

} // namespace grenade::vx::network
