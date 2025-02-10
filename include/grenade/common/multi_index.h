#pragma once
#include "grenade/common/genpybind.h"
#include "hate/visibility.h"
#include <cstddef>
#include <iosfwd>
#include <vector>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Multi index in index space.
 * It is used to describe a point in an index space.
 *
 * For example, when representing data-flow across a two-dimensional collection of channels, one
 * channel can be represented by a multi index like (3, 7).
 */
struct GENPYBIND(visible) MultiIndex
{
	MultiIndex() = default;
	MultiIndex(std::vector<size_t> value) SYMBOL_VISIBLE;

	/**
	 * Value of multi index.
	 * For each dimension, the index is given.
	 * Indices are integer numbers >= 0.
	 */
	std::vector<size_t> value;

	auto operator<=>(MultiIndex const& other) const = default;

	friend std::ostream& operator<<(std::ostream& os, MultiIndex const& value) SYMBOL_VISIBLE;
};

} // namespace common
} // namespace grenade
