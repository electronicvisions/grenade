#pragma once
#include "halco/common/typed_array.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/ppu.h"
#include "hate/visibility.h"
#include <cstdint>
#include <string>
#include <vector>

namespace grenade::vx {

/**
 * Convert column byte values to PPUMemoryBlock.
 */
haldls::vx::v3::PPUMemoryBlock to_vector_unit_row(
    halco::common::typed_array<int8_t, halco::hicann_dls::vx::v3::NeuronColumnOnDLS> const& values)
    SYMBOL_VISIBLE;

/**
 * Convert PPUMemoryBlock to column byte values.
 */
halco::common::typed_array<int8_t, halco::hicann_dls::vx::v3::NeuronColumnOnDLS>
from_vector_unit_row(haldls::vx::v3::PPUMemoryBlock const& values) SYMBOL_VISIBLE;

inline static std::string const ppu_program_name = "grenade_ppu_base_vx";

/**
 * Get full path to program specified. This function searches the PATH for the given name.
 * @param name Name to search for
 * @return String of full path to program
 */
std::string get_program_path(std::string const& name) SYMBOL_VISIBLE;

} // namespace grenade::vx
