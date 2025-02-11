#include "grenade/common/projection_connector/static.h"

namespace grenade::common {

bool StaticConnector::valid(size_t, Projection::Connector::Parameterization const&) const
{
	return false;
}

bool StaticConnector::valid(size_t, Projection::Connector::Dynamics const&) const
{
	return false;
}

size_t StaticConnector::get_num_synapses(MultiIndexSequence const& sequence) const
{
	return get_synapse_connections(sequence)->size();
}

std::vector<Vertex::Port> StaticConnector::get_input_ports() const
{
	return {};
}

std::vector<Vertex::Port> StaticConnector::get_output_ports() const
{
	return {};
}

} // namespace grenade::common
