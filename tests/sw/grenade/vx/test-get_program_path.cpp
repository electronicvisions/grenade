#include "grenade/vx/ppu.h"
#include <gtest/gtest.h>


TEST(get_program_path, PPUProgram)
{
	EXPECT_NO_THROW(grenade::vx::get_program_path(grenade::vx::ppu_program_name));
}
