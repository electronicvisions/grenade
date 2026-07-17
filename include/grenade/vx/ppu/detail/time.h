#pragma once
#ifdef __ppu__
#include "hate/visibility.h"
#include "libnux/vx/attrib.h"

namespace grenade::vx::ppu::detail {

/**
 * Initialize time origin to current time.
 */
void initialize_time_origin() SYMBOL_VISIBLE ATTRIB_LINK_TO_INTERNAL;

} // namespace grenade::vx::ppu::detail
#endif
