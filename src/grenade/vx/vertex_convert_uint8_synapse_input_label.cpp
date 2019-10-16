#include "grenade/vx/vertex/convert_uint8_synapse_input_label.h"

namespace grenade::vx::vertex {

ConvertInt8ToSynapseInputLabel::ConvertInt8ToSynapseInputLabel(size_t const size) : m_size(size) {}

std::array<Port, 1> ConvertInt8ToSynapseInputLabel::inputs() const
{
	return {Port(m_size, ConnectionType::Int8)};
}

Port ConvertInt8ToSynapseInputLabel::output() const
{
	return Port(m_size, ConnectionType::SynapseInputLabel);
}

} // namespace grenade::vx::vertex
