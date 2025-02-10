#pragma once
#include "grenade/common/vertex_port_type.h"
#include "grenade/common/vertex_port_type/empty.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Synaptic input port to be used for connection from synapses to neurons.
 */
struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) SynapticInput
    : public dapr::EmptyProperty<SynapticInput, grenade::common::VertexPortType>
{};

} // namespace abstract
} // namespace grenade::vx::network
