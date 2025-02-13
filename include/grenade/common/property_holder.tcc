#pragma once
#include "grenade/common/property_holder.h"
#include <ostream>
#include <stdexcept>

namespace grenade::common {

template <typename T, template <typename...> typename Backend>
PropertyHolder<T, Backend>::PropertyHolder(T const& value) : m_backend(value.copy())
{}

template <typename T, template <typename...> typename Backend>
PropertyHolder<T, Backend>::PropertyHolder(T&& value) : m_backend(value.move())
{}

template <typename T, template <typename...> typename Backend>
PropertyHolder<T, Backend>::PropertyHolder(PropertyHolder const& other) :
    m_backend(other ? other->copy() : nullptr)
{}

template <typename T, template <typename...> typename Backend>
PropertyHolder<T, Backend>& PropertyHolder<T, Backend>::operator=(PropertyHolder const& other)
{
	if (this != &other) {
		m_backend = other.m_backend ? other.m_backend->copy() : nullptr;
	}
	return *this;
}

template <typename T, template <typename...> typename Backend>
PropertyHolder<T, Backend>& PropertyHolder<T, Backend>::operator=(T const& other)
{
	m_backend = other.copy();
	return *this;
}

template <typename T, template <typename...> typename Backend>
PropertyHolder<T, Backend>& PropertyHolder<T, Backend>::operator=(T&& other)
{
	m_backend = other.move();
	return *this;
}

template <typename T, template <typename...> typename Backend>
PropertyHolder<T, Backend>::operator bool() const
{
	return static_cast<bool>(m_backend);
}

template <typename T, template <typename...> typename Backend>
T& PropertyHolder<T, Backend>::operator*() const
{
	if (!m_backend) {
		throw std::runtime_error("Trying to dereference unset property holder.");
	}
	return *m_backend;
}

template <typename T, template <typename...> typename Backend>
T* PropertyHolder<T, Backend>::operator->() const
{
	if (!m_backend) {
		throw std::runtime_error("Trying to dereference unset property holder.");
	}
	return m_backend.operator->();
}

template <typename T, template <typename...> typename Backend>
bool PropertyHolder<T, Backend>::operator==(PropertyHolder const& other) const
{
	if (static_cast<bool>(m_backend) != static_cast<bool>(other.m_backend)) {
		return false;
	}
	return !m_backend || (*m_backend == *(other.m_backend));
}

template <typename T, template <typename...> typename Backend>
bool PropertyHolder<T, Backend>::operator!=(PropertyHolder const& other) const
{
	return !(*this == other);
}

template <typename T, template <typename...> typename Backend>
std::ostream& operator<<(std::ostream& os, PropertyHolder<T, Backend> const& value)
{
	return os << *value;
}

} // namespace grenade::common
