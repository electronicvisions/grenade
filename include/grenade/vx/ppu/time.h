#pragma once
#include "hate/visibility.h"
#include "libnux/vx/attrib.h"
#include <cstdint>

namespace grenade::vx::ppu {

/**
 * Get time since start of the experiment in PPU clock cycles.
 */
uint64_t now() SYMBOL_VISIBLE ATTRIB_LINK_TO_INTERNAL;

} // namespace grenade::vx::ppu
