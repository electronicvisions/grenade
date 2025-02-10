#include "grenade/common/partitioned_vertex.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/optional.hpp>
#include <cereal/types/polymorphic.hpp>

namespace grenade::common {

template <typename Archive>
void PartitionedVertex::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<Vertex>(this));
	ar(m_execution_instance_on_executor);
}

} // namespace grenade::common

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::PartitionedVertex)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::common::PartitionedVertex)
CEREAL_CLASS_VERSION(grenade::common::PartitionedVertex, 0)
CEREAL_REGISTER_TYPE(grenade::common::PartitionedVertex)
CEREAL_REGISTER_POLYMORPHIC_RELATION(grenade::common::Vertex, grenade::common::PartitionedVertex)
