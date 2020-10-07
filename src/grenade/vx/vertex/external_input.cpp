#include "grenade/vx/vertex/external_input.h"

#include "grenade/cerealization.h"
#include <stdexcept>

namespace grenade::vx::vertex {

ExternalInput::ExternalInput(ConnectionType const output_type, size_t const size) :
    m_size(size), m_output_type(output_type)
{}

Port ExternalInput::output() const
{
	return Port(m_size, m_output_type);
}

std::ostream& operator<<(std::ostream& os, ExternalInput const& config)
{
	os << "ExternalInput(size: " << config.m_size << ", output_type: " << config.m_output_type
	   << ")";
	return os;
}

bool ExternalInput::operator==(ExternalInput const& other) const
{
	return (m_size == other.m_size) && (m_output_type == other.m_output_type);
}

bool ExternalInput::operator!=(ExternalInput const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void ExternalInput::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_size);
	ar(m_output_type);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::ExternalInput)
CEREAL_CLASS_VERSION(grenade::vx::vertex::ExternalInput, 0)
