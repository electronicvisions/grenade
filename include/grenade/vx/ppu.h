#pragma once
#include "haldls/vx/v2/ppu.h"
#include "hate/visibility.h"
#include <cstdint>
#include <string>
#include <vector>

namespace grenade::vx {

/**
 * Convert byte values indexed via NeuronColumnOnDLS to PPUMemoryBlock.
 */
haldls::vx::v2::PPUMemoryBlock to_vector_unit_row(std::vector<int8_t> const& values) SYMBOL_VISIBLE;

/**
 * Convert PPUMemoryBlock to byte values indexed via SynapseOnSynapseRow.
 */
std::vector<int8_t> from_vector_unit_row(haldls::vx::v2::PPUMemoryBlock const& values)
    SYMBOL_VISIBLE;

inline static std::string const ppu_program_name = "grenade_ppu_base_vx";

/**
 * Get full path to program specified. This function searches the PATH for the given name.
 * @param name Name to search for
 * @return String of full path to program
 */
std::string get_program_path(std::string const& name) SYMBOL_VISIBLE;

} // namespace grenade::vx
