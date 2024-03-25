#pragma once
#include "hate/visibility.h"

namespace grenade::vx::network {

/**
 * Object which is equality comparable.
 * @tparam Derived class
 */
template <typename T>
struct SYMBOL_VISIBLE EqualityComparable
{
	/**
	 * Compare equal to other derived object.
	 * @param other Other derived object.
	 */
	bool operator==(EqualityComparable const& other) const;

	/**
	 * Compare unequal to other derived object.
	 * @param other Other derived object.
	 */
	bool operator!=(EqualityComparable const& other) const;

protected:
	/**
	 * Check whether this instance is equal to the other object.
	 * This method is expected to only be called in the operator==.
	 * It is allowed to assume that the other object is of the same type as this instance.
	 * @param other Other derived object
	 */
	virtual bool is_equal_to(T const& other) const = 0;
};

} // namespace grenade::vx::network

#include "grenade/vx/network/abstract/equality_comparable.tcc"
