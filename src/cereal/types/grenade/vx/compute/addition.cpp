#include "grenade/vx/compute/addition.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::vx::compute {

template <typename Archive>
void Addition::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_topology);
	ar(m_vertex);
	ar(m_other);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::Addition)
CEREAL_CLASS_VERSION(grenade::vx::compute::Addition, 0)
