#pragma once
#include "grenade/common/genpybind.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade::common {

/**
 * Object which is copyable.
 * @tparam T Derived class
 */
template <typename T>
struct SYMBOL_VISIBLE Copyable
{
	/**
	 * Copy object.
	 */
	virtual std::unique_ptr<T> copy() const GENPYBIND(hidden) = 0;
};

} // namespace grenade::common
