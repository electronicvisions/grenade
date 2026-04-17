#include "grenade/vx/compute/relu.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/memory.hpp>

namespace grenade::vx::compute {

template <typename Archive>
void ReLU::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_graph);
	ar(m_vertex);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::ReLU)
CEREAL_CLASS_VERSION(grenade::vx::compute::ReLU, 0)
