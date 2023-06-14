#include "grenade/vx/signal_flow/vertex/data_input.h"

#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void DataInput::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
	ar(m_output_type);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::DataInput)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::DataInput, 0)
