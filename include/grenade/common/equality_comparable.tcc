#pragma once
#include "grenade/common/equality_comparable.h"

#include <typeinfo>

namespace grenade::common {

template <typename T>
bool EqualityComparable<T>::operator==(EqualityComparable const& other) const
{
	if (typeid(*this) != typeid(other)) {
		return false;
	}
	return is_equal_to(static_cast<T const&>(other));
}

template <typename T>
bool EqualityComparable<T>::operator!=(EqualityComparable const& other) const
{
	return !operator==(other);
}

} // namespace grenade::common
