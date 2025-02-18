#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/multi_index_sequence.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Dimension unit of cells on populations.
 */
struct GENPYBIND(inline_base("*EmptyProperty*")) CellOnPopulationDimensionUnit
    : public dapr::EmptyProperty<CellOnPopulationDimensionUnit, MultiIndexSequence::DimensionUnit>
{};

} // namespace common
} // namespace grenade
