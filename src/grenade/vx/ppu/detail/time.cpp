#include "grenade/vx/ppu/detail/time.h"

#ifdef __ppu__
#include "libnux/vx/time.h"

namespace grenade::vx::ppu::detail {

uint64_t time_origin;

void initialize_time_origin()
{
	time_origin = libnux::vx::now();
}

} // namespace grenade::vx::ppu::detail
#endif
