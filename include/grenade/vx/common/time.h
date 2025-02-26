#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/common/geometry.h"
#include "haldls/vx/v3/event.h"
#include "haldls/vx/v3/timer.h"
#include "hate/visibility.h"

namespace grenade::vx {
namespace common GENPYBIND_TAG_GRENADE_VX_COMMON {

/**
 * Arithmetic type used for time measurements both to and from the hardware.
 * Representation is implemented as 64 bit wide unsigned integer of units [FPGA clock cycles].
 * TODO: Think about using std::chrono instead of our wrapper type.
 */
struct GENPYBIND(inline_base("*")) Time : public halco::common::detail::BaseType<Time, uint64_t>
{
	constexpr explicit Time(value_type const value = 0) GENPYBIND(implicit_conversion) :
	    base_t(value)
	{}

	static const Time fpga_clock_cycles_per_us SYMBOL_VISIBLE;

	haldls::vx::v3::FPGATime toFPGATime() const SYMBOL_VISIBLE;
	haldls::vx::v3::ChipTime toChipTime() const SYMBOL_VISIBLE;
	haldls::vx::v3::Timer::Value toTimerOnFPGAValue() const SYMBOL_VISIBLE;
};

} // namespace common
} // namespace grenade::vx
