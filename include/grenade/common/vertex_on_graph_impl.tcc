#include "grenade/common/vertex_on_graph.h"

#include "hate/type_index.h"
#include <ostream>

namespace grenade::common {

template <typename Derived, typename Backend>
VertexOnGraph<Derived, Backend>::VertexOnGraph(Value const& value) : m_value(value)
{}

template <typename Derived, typename Backend>
bool VertexOnGraph<Derived, Backend>::operator<(Derived const& other) const
{
	return m_value < other.m_value;
}

template <typename Derived, typename Backend>
bool VertexOnGraph<Derived, Backend>::operator<=(Derived const& other) const
{
	return m_value <= other.m_value;
}

template <typename Derived, typename Backend>
bool VertexOnGraph<Derived, Backend>::operator>(Derived const& other) const
{
	return m_value > other.m_value;
}

template <typename Derived, typename Backend>
bool VertexOnGraph<Derived, Backend>::operator>=(Derived const& other) const
{
	return m_value >= other.m_value;
}

template <typename Derived, typename Backend>
bool VertexOnGraph<Derived, Backend>::operator==(Derived const& other) const
{
	return m_value == other.m_value;
}

template <typename Derived, typename Backend>
bool VertexOnGraph<Derived, Backend>::operator!=(Derived const& other) const
{
	return m_value != other.m_value;
}

template <typename Derived, typename Backend>
typename VertexOnGraph<Derived, Backend>::Value const& VertexOnGraph<Derived, Backend>::value()
    const
{
	return m_value;
}

template <typename Derived, typename Backend>
size_t VertexOnGraph<Derived, Backend>::hash() const
{
	// We include the type name in the hash to reduce the number of hash collisions in
	// python code, where __hash__ is used in heterogeneous containers.
	static const size_t seed = boost::hash_value(typeid(VertexOnGraph).name());
	size_t hash = seed;
	boost::hash_combine(hash, std::hash<VertexOnGraph>{}(*this));
	return hash;
}

template <typename Derived, typename Backend>
std::ostream& VertexOnGraph<Derived, Backend>::print(std::ostream& os) const
{
	return (os << hate::name<Derived>() << "(" << m_value << ")");
}

} // namespace grenade::common
