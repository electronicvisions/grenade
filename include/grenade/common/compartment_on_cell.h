#pragma once
#include "grenade/common/genpybind.h"
#include "halco/common/geometry.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Identifier of compartment on a multi-compartment cell.
 */
struct GENPYBIND(inline_base("*")) CompartmentOnCell
    : public halco::common::detail::BaseType<CompartmentOnCell, size_t>
{
	constexpr explicit CompartmentOnCell(value_type const value = 0) : base_t(value) {}
};

} // namespace common
} // namespace grenade
