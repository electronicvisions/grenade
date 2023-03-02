#include "grenade/vx/common/time.h"

#include "fisch/vx/constants.h"

#include <stdexcept>
#include <string>

namespace grenade::vx::common {

Time const Time::fpga_clock_cycles_per_us{fisch::vx::fpga_clock_cycles_per_us};

haldls::vx::v3::FPGATime Time::toFPGATime() const
{
	try {
		return haldls::vx::v3::FPGATime(value());
	} catch (std::out_of_range const& error) {
		throw std::out_of_range(
		    "Error in conversion grenade::vx::common::Time::toFPGATime(): " +
		    std::string(error.what()));
	}
}

haldls::vx::v3::ChipTime Time::toChipTime() const
{
	try {
		return haldls::vx::v3::ChipTime(value());
	} catch (std::out_of_range const& error) {
		throw std::out_of_range(
		    "Error in conversion grenade::vx::common::Time::toChipTime(): " +
		    std::string(error.what()));
	}
}

haldls::vx::v3::Timer::Value Time::toTimerOnFPGAValue() const
{
	try {
		return haldls::vx::v3::Timer::Value(value());
	} catch (std::out_of_range const& error) {
		throw std::out_of_range(
		    "Error in conversion grenade::vx::common::Time::toTimerOnFPGAValue(): " +
		    std::string(error.what()));
	}
}

} // namespace grenade::vx::common
