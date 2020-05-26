#include "grenade/vx/vertex/external_input.h"

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

} // namespace grenade::vx
