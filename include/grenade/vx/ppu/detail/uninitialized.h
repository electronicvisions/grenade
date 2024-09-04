#pragma once

namespace grenade::vx::ppu::detail {

template <typename T>
using uninitialized __attribute__((aligned(__alignof__(T)))) = char[sizeof(T)];

template <typename T>
constexpr inline T& uninitialized_cast(uninitialized<T>& raw)
{
	return *reinterpret_cast<T*>(&raw);
}

} // namespace grenade::vx::ppu::detail
