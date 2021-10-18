#include <gtest/gtest.h>

#include "grenade/vx/ppu.h"
#include "halco/hicann-dls/vx/v2/ppu.h"
#include "haldls/vx/v2/ppu.h"

TEST(from_vector_unit_row, PPUProgram)
{
	constexpr static size_t words_per_row = 256 / sizeof(uint32_t);
	{
		haldls::vx::v2::PPUMemoryBlock values{
		    halco::hicann_dls::vx::v2::PPUMemoryBlockSize(words_per_row)};
		EXPECT_EQ(
		    grenade::vx::from_vector_unit_row(values).size(), values.size() * sizeof(uint32_t));
	}
	{
		haldls::vx::v2::PPUMemoryBlock values{
		    halco::hicann_dls::vx::v2::PPUMemoryBlockSize(words_per_row - 1)};
		EXPECT_THROW(grenade::vx::from_vector_unit_row(values), std::runtime_error);
	}
	{
		haldls::vx::v2::PPUMemoryBlock values{
		    halco::hicann_dls::vx::v2::PPUMemoryBlockSize(words_per_row + 1)};
		EXPECT_THROW(grenade::vx::from_vector_unit_row(values), std::runtime_error);
	}
}
