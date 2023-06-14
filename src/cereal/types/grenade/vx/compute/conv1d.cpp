#include "grenade/vx/compute/conv1d.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/vector.hpp>

namespace grenade::vx::compute {

template <typename Archive>
void Conv1d::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_enable_loopback);
	ar(m_input_size);
	ar(m_kernel_size);
	ar(m_in_channels);
	ar(m_out_channels);
	ar(m_stride);
	ar(m_mac);
	ar(m_num_sends);
	ar(m_wait_between_events);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::Conv1d)
CEREAL_CLASS_VERSION(grenade::vx::compute::Conv1d, 0)
