#pragma once
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade::vx::network {

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

} // namespace grenade::vx::network
