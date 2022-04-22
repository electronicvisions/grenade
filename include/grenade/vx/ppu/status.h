#pragma once
#include <cstdint>

namespace grenade::vx::ppu {

enum class Status : uint32_t
{
	idle,
	reset_neurons,
	baseline_read,
	read,
	periodic_read,
	inside_periodic_read,
	stop_periodic_read,
	stop
};

} // namespace grenade::vx::ppu
