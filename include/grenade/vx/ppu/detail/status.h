#pragma once
#include <cstdint>
#ifndef __ppu__
#include "hate/visibility.h"
#include <iosfwd>
#endif

namespace grenade::vx::ppu::detail {

enum class Status : uint32_t
{
	initial,
	idle,
	reset_neurons,
	baseline_read,
	read,
	periodic_read,
	inside_periodic_read,
	stop_periodic_read,
	stop,
	scheduler
};

#ifndef __ppu__
std::ostream& operator<<(std::ostream& os, Status const& value) SYMBOL_VISIBLE;
#endif

} // namespace grenade::vx::ppu::detail
