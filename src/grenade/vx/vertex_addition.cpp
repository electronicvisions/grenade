#include "grenade/vx/vertex/addition.h"

#include <stdexcept>

namespace grenade::vx::vertex {

Addition::Addition(size_t const size, size_t const num_inputs) :
    m_size(size), m_num_inputs(num_inputs)
{}

std::vector<Port> Addition::inputs() const
{
	Port const port(m_size, ConnectionType::Int8);
	std::vector<Port> ret;
	for (size_t i = 0; i < m_num_inputs; ++i) {
		ret.emplace_back(port);
	}
	return ret;
}

Port Addition::output() const
{
	return Port(m_size, ConnectionType::Int8);
}

} // namespace grenade::vx::vertex
