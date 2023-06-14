#include "grenade/vx/signal_flow/vertex/data_output.h"

#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void DataOutput::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
	ar(m_input_type);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::DataOutput)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::DataOutput, 0)
