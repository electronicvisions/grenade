#pragma once
#include <utility>

namespace grenade::common::detail {

template <typename T>
struct ConstructorTransform
{
	template <typename U>
	constexpr T operator()(U&& value) const
	{
		return T(std::forward<U>(value));
	}
};

} // namespace grenade::common::detail
