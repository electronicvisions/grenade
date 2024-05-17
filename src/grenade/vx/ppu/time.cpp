#include "grenade/vx/ppu/time.h"

#include "libnux/vx/time.h"

namespace grenade::vx::ppu {

namespace detail {

extern uint64_t time_origin;

} // namespace detail

uint64_t now()
{
	return libnux::vx::now() - detail::time_origin;
}

} // namespace grenade::vx::ppu
