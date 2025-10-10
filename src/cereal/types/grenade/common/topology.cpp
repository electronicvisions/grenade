#include "grenade/common/topology.h"

#include "cereal/types/grenade/common/graph.h"
#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"

namespace grenade::common {

template <typename Archive>
void Topology::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<Graph>(this));
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::Topology)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::Topology)
CEREAL_CLASS_VERSION(grenade::common::Topology, 0)
