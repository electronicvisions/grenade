#pragma once
#include "halco/common/geometry.h"
#include "haldls/vx/v2/padi.h"

namespace grenade::vx {

/**
 * 5 bit wide unsigned activation value type.
 */
typedef haldls::vx::v2::PADIEvent::HagenActivation UInt5;

/**
 * 32 bit wide unsigned integer value of e.g. indices.
 */
struct UInt32 : public halco::common::detail::BaseType<UInt32, uint32_t>
{
	constexpr explicit UInt32(value_type const value = 0) : base_t(value) {}
};

/**
 * 8 bit wide signed integer value of e.g. CADC membrane readouts.
 *
 * In haldls we use unsigned values for the CADC readouts, this shall be a typesafe wrapper around
 * int8_t, which we don't currently have, since it relies on the interpretation of having the CADC
 * baseline in the middle of the value range.
 */
struct Int8 : public halco::common::detail::BaseType<Int8, int8_t>
{
	constexpr explicit Int8(value_type const value = 0) : base_t(value) {}
};

} // namespace grenade::vx
