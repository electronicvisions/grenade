#pragma once
#include "grenade/common/genpybind.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade::common {

/**
 * Object which is movable.
 * @tparam T Derived class
 */
template <typename T>
struct SYMBOL_VISIBLE Movable
{
	/**
	 * Move object.
	 */
	virtual std::unique_ptr<T> move() GENPYBIND(hidden) = 0;
};

} // namespace grenade::common
