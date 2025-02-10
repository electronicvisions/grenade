#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/vertex_port_type.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Weight parameterization port type.
 */
struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) Weight
    : public dapr::EmptyProperty<Weight, grenade::common::VertexPortType>
{};

} // namespace abstract
} // namespace grenade::vx::network
