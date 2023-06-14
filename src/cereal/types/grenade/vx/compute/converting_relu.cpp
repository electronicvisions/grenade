#include "grenade/vx/compute/converting_relu.h"

#include "grenade/cerealization.h"

namespace grenade::vx::compute {

template <typename Archive>
void ConvertingReLU::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_graph);
	ar(m_input_vertex);
	ar(m_output_vertex);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::ConvertingReLU)
CEREAL_CLASS_VERSION(grenade::vx::compute::ConvertingReLU, 0)
