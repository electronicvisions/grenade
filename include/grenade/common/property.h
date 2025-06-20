#pragma once
#include "grenade/common/copyable.h"
#include "grenade/common/equality_comparable.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/movable.h"
#include "grenade/common/printable.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {
/**
 * Property, which we define to have the following properties:
 *  - copyable
 *  - movable
 *  - printable
 *  - equality-comparable
 */
template <typename Derived>
struct SYMBOL_VISIBLE Property
    : public Copyable<Derived>
    , public Movable<Derived>
    , public Printable
    , public EqualityComparable<Derived>
{};
} // namespace common
} // namespace grenade
