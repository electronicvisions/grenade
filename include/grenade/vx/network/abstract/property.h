#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/copyable.h"
#include "grenade/vx/network/abstract/equality_comparable.h"
#include "grenade/vx/network/abstract/movable.h"
#include "grenade/vx/network/abstract/printable.h"

namespace grenade::vx { namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

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

} // namespace network
} // namespace grenade::vx
