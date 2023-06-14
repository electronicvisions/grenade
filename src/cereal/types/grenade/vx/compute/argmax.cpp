#include "grenade/vx/compute/argmax.h"

#include "grenade/cerealization.h"

namespace grenade::vx::compute {

template <typename Archive>
void ArgMax::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_graph);
	ar(m_input_vertex);
	ar(m_output_vertex);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::ArgMax)
CEREAL_CLASS_VERSION(grenade::vx::compute::ArgMax, 0)
