#pragma once
#include "grenade/common/property_holder.h"
#include <utility>

namespace grenade::common::detail {

template <typename Key, typename Value>
struct MapTransform
{
	template <typename U>
	constexpr std::pair<Key const&, Value const&> operator()(U&& value) const
	{
		if (!value.second) {
			throw std::logic_error("Unexpected access to moved-from object.");
		}
		return std::pair<Key const&, Value const&>{value.first, *value.second};
	}
};

} // namespace grenade::common::detail
