#pragma once
#include <cstdint>

namespace grenade::vx::ppu {

enum class Status : uint32_t
{
	idle,
	reset_neurons,
	baseline_read,
	read,
	stop
};

} // namespace grenade::vx::ppu
