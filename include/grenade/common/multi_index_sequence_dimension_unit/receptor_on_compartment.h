#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/multi_index_sequence.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Dimension unit of neurons on populations.
 */
struct GENPYBIND(inline_base("*EmptyProperty*")) ReceptorOnCompartmentDimensionUnit
    : public dapr::
          EmptyProperty<ReceptorOnCompartmentDimensionUnit, MultiIndexSequence::DimensionUnit>
{};

} // namespace common
} // namespace grenade
