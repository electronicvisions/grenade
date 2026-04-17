#include "grenade/vx/compute/mac.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::vx::compute {

template <typename Archive>
void MAC::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_enable_loopback);
	ar(m_topology);
	ar(m_parameterization);
	ar(m_neuron_vertices);
	ar(m_input_vertex);
	ar(m_output_vertex);
	ar(m_weights);
	ar(m_num_sends);
	ar(m_wait_between_events);
	ar(m_madc_recording_neuron);
	ar(m_madc_recording_path);
	ar(m_madc_recording_vertices);
	ar(m_chip_vertices);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::MAC)
CEREAL_CLASS_VERSION(grenade::vx::compute::MAC, 0)
