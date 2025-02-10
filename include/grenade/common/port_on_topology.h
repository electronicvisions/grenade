#pragma once
#include "grenade/common/edge.h"
#include "grenade/common/vertex_on_topology.h"
#include "hate/visibility.h"
#include <utility>

namespace grenade::common {

typedef std::pair<VertexOnTopology, Edge::PortOnVertex> PortOnTopology;

} // namespace grenade::common

namespace std {

std::ostream& operator<<(std::ostream& os, grenade::common::PortOnTopology const& value)
    SYMBOL_VISIBLE;

} // namespace std
