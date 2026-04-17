#pragma once
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"


/**
 * Generate lola chip with all neurons in excitatory bypass configuration.
 */
lola::vx::v3::Chip get_chip_config_bypass_excitatory() SYMBOL_VISIBLE;
