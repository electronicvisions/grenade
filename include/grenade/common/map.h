#pragma once
#include "grenade/common/detail/map_transform.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/property.h"
#include "grenade/common/property_holder.h"
#include <map>
#include <boost/iterator/transform_iterator.hpp>

namespace grenade::common {

/**
 * Map storing potentially polymorphic values.
 * @tparam KeyT Key type
 * @tparam ValueT Value type required to be derived from a Property
 */
template <typename KeyT, typename ValueT>
struct GENPYBIND(visible) Map
{
	typedef KeyT Key;
	typedef ValueT Value;
	typedef std::map<Key, PropertyHolder<Value>> Backend;

	Map() = default;

	/**
	 * Add element to map.
	 * @param key Key for which to add value
	 * @param value Value to add
	 * @throws std::out_of_range On map already containing entry for given key
	 */
	void add(Key const& key, Value const& value);

	/**
	 * Add element to map.
	 * @param key Key for which to add value
	 * @param value Value to add
	 * @throws std::out_of_range On map already containing entry for given key
	 */
	void add(Key const& key, Value&& value) GENPYBIND(hidden);

	/**
	 * Get value of element present in map.
	 * @param key Key for which to get value
	 * @return Value to get
	 * @throws std::out_of_range On no element for key present in map
	 */
	Value const& get(Key const& key) const GENPYBIND(hidden);

	GENPYBIND_MANUAL({
		typedef typename decltype(parent)::type self_type;
		parent.def(
		    "get",
		    [](GENPYBIND_PARENT_TYPE const& self, typename self_type::Key const& key) ->
		    typename self_type::Value const& { return self.get(key); },
		    parent->py::return_value_policy::reference_internal);
	})

	/**
	 * Set value of element present in map.
	 * @param key Key for which to get value
	 * @param value Value to get
	 * @throws std::out_of_range On no element for key present in map
	 */
	void set(Key const& key, Value const& value);

	/**
	 * Set value of element present in map.
	 * @param key Key for which to get value
	 * @param value Value to get
	 * @throws std::out_of_range On no element for key present in map
	 */
	void set(Key const& key, Value&& value) GENPYBIND(hidden);

	/**
	 * Erase element for given key in map.
	 * @param key Key to erase element for
	 */
	void erase(Key const& key);

	/**
	 * Get whether element for given key is present in map.
	 * @param key Key to get property for
	 */
	bool contains(Key const& key) const;

	/**
	 * Get number of elements present in map.
	 */
	size_t size() const;

	/**
	 * Get whether map is empty and contains no elements.
	 */
	bool empty() const;

	typedef boost::transform_iterator<
	    typename detail::MapTransform<Key, Value>,
	    typename Backend::const_iterator>
	    ConstIterator;

	/**
	 * Get begin iterator to elements in map.
	 */
	ConstIterator begin() const GENPYBIND(hidden);

	/**
	 * Get end iterator to elements in map.
	 */
	ConstIterator end() const GENPYBIND(hidden);

	bool operator==(Map const& other) const = default;
	bool operator!=(Map const& other) const = default;

private:
	static_assert(std::is_base_of_v<Property<Value>, Value>);
	Backend m_values;
};

} // namespace grenade::common

#include "grenade/common/map.tcc"
