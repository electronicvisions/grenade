#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/vertex_port_type.h"

namespace grenade::common {

/**
 * Vertex port type without content for using when the type information alone suffices.
 */
template <typename Derived>
struct EmptyVertexPortType : public dapr::EmptyProperty<Derived, VertexPortType>
{};

} // namespace grenade::common
