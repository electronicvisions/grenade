#pragma once
#include "halco/common/typed_array.h"
#include "haldls/vx/v2/neuron.h"
#include "haldls/vx/v2/ppu.h"
#include "hate/visibility.h"
#include "lola/vx/v2/ppu.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace grenade::vx {

/**
 * Convert column byte values to PPUMemoryBlock.
 */
haldls::vx::v2::PPUMemoryBlock to_vector_unit_row(
    halco::common::typed_array<int8_t, halco::hicann_dls::vx::v2::NeuronColumnOnDLS> const& values)
    SYMBOL_VISIBLE;

/**
 * Convert PPUMemoryBlock to column byte values.
 */
halco::common::typed_array<int8_t, halco::hicann_dls::vx::v2::NeuronColumnOnDLS>
from_vector_unit_row(haldls::vx::v2::PPUMemoryBlock const& values) SYMBOL_VISIBLE;

/**
 * Create (and automatically delete) temporary directory.
 */
struct TemporaryDirectory
{
	TemporaryDirectory(std::string directory_template) SYMBOL_VISIBLE;
	~TemporaryDirectory() SYMBOL_VISIBLE;

	std::filesystem::path get_path() const SYMBOL_VISIBLE;

private:
	std::filesystem::path m_path;
};

/**
 * Get full path to linker file.
 * @param name Name to search for
 */
std::string get_linker_file(std::string const& name) SYMBOL_VISIBLE;

/**
 * Get include paths.
 */
std::string get_include_paths() SYMBOL_VISIBLE;

/**
 * Get library paths.
 */
std::string get_library_paths() SYMBOL_VISIBLE;

/**
 * Get full path to libnux runtime.
 * @param name Name to search for
 */
std::string get_libnux_runtime(std::string const& name) SYMBOL_VISIBLE;

/**
 * Get full path to PPU program source.
 */
std::string get_program_base_source() SYMBOL_VISIBLE;

/**
 * Compiler for PPU programs.
 */
struct Compiler
{
	static constexpr auto name = "powerpc-ppu-g++";

	Compiler() SYMBOL_VISIBLE;

	std::vector<std::string> options_before_source = {
	    "-std=gnu++17",
	    "-fdiagnostics-color=always",
	    "-O2",
	    "-g",
	    "-fno-omit-frame-pointer",
	    "-fno-strict-aliasing",
	    "-Wall",
	    "-Wextra",
	    "-pedantic",
	    "-ffreestanding",
	    "-mcpu=nux",
	    "-fno-exceptions",
	    "-fno-rtti",
	    "-fno-non-call-exceptions",
	    "-fno-common",
	    "-ffunction-sections",
	    "-fdata-sections",
	    "-fno-threadsafe-statics",
	    "-mcpu=s2pp_hx",
	    get_include_paths(),
	    "-DSYSTEM_HICANN_DLS_MINI",
	    "-fuse-ld=bfd",
	    "-Wl,--gc-sections",
	    "-nostdlib",
	    "-T" + get_linker_file("elf32nux.x"),
	    "-Wl,--defsym=mailbox_size=4096",
	};

	std::vector<std::string> options_after_source = {
	    "-Bstatic",
	    get_library_paths(),
	    "-lgcc",
	    "-lnux_vx_v2",
	    "-Wl,--whole-archive",
	    get_libnux_runtime("nux_runtime_vx_v2.o"),
	    "-Wl,--no-whole-archive",
	    "-Bdynamic",
	};

	/**
	 * Compile sources into target program.
	 */
	std::pair<lola::vx::v2::PPUElfFile::symbols_type, haldls::vx::v2::PPUMemoryBlock> compile(
	    std::vector<std::string> sources) SYMBOL_VISIBLE;
};

} // namespace grenade::vx
