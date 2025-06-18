#pragma once
#include "grenade/common/map.h"
#include <cassert>
#include <sstream>

namespace grenade::common {

template <typename Key, typename Value>
Value const& Map<Key, Value>::get(Key const& key) const
{
	if (!m_values.contains(key)) {
		std::stringstream ss;
		ss << "Map doesn't contain entry for key " << key << ".";
		throw std::out_of_range(ss.str());
	}
	return *m_values.at(key);
}

template <typename Key, typename Value>
void Map<Key, Value>::set(Key const& key, Value const& value)
{
	if (!m_values.contains(key)) {
		m_values.emplace(key, value);
	} else {
		m_values.at(key) = value;
	}
}

template <typename Key, typename Value>
void Map<Key, Value>::set(Key const& key, Value&& value)
{
	if (!m_values.contains(key)) {
		m_values.emplace(key, std::move(value));
	} else {
		m_values.at(key) = std::move(value);
	}
}

template <typename Key, typename Value>
void Map<Key, Value>::erase(Key const& key)
{
	m_values.erase(key);
}

template <typename Key, typename Value>
bool Map<Key, Value>::contains(Key const& key) const
{
	return m_values.contains(key);
}

template <typename Key, typename Value>
size_t Map<Key, Value>::size() const
{
	return m_values.size();
}

template <typename Key, typename Value>
bool Map<Key, Value>::empty() const
{
	return m_values.empty();
}

template <typename Key, typename Value>
typename Map<Key, Value>::ConstIterator Map<Key, Value>::begin() const
{
	return ConstIterator(m_values.begin());
}

template <typename Key, typename Value>
typename Map<Key, Value>::ConstIterator Map<Key, Value>::end() const
{
	return ConstIterator(m_values.end());
}

} // namespace grenade::common
