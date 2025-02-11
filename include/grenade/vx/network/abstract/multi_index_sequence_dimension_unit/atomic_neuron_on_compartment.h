#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/vx/genpybind.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Dimension unit of neurons on populations.
 */
struct GENPYBIND(inline_base("*EmptyProperty*")) AtomicNeuronOnCompartmentDimensionUnit
    : public dapr::EmptyProperty<
          AtomicNeuronOnCompartmentDimensionUnit,
          grenade::common::MultiIndexSequence::DimensionUnit>
{};

} // namespace abstract
} // namespace grenade::vx::network
