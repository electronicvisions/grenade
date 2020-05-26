#include "grenade/vx/vertex/synapse_driver.h"

#include "grenade/vx/vertex/padi_bus.h"

namespace grenade::vx::vertex {

SynapseDriver::SynapseDriver(
    Coordinate const& coordinate, Config const& config, RowModes const& row_modes) :
    m_coordinate(coordinate), m_config(config), m_row_modes(row_modes)
{}

SynapseDriver::Coordinate SynapseDriver::get_coordinate() const
{
	return m_coordinate;
}

SynapseDriver::Config SynapseDriver::get_config() const
{
	return m_config;
}

SynapseDriver::RowModes SynapseDriver::get_row_modes() const
{
	return m_row_modes;
}

bool SynapseDriver::supports_input_from(PADIBus const& input) const
{
	return halco::hicann_dls::vx::PADIBusOnDLS(
	           m_coordinate.toSynapseDriverOnSynapseDriverBlock().toPADIBusOnPADIBusBlock(),
	           m_coordinate.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS()) ==
	       input.get_coordinate();
}

std::ostream& operator<<(std::ostream& os, SynapseDriver const& config)
{
	os << "SynapseDriver(coordinate: " << config.m_coordinate << ", row_mask: " << config.m_config
	   << ", row_modes: {top("
	   << config.m_row_modes[halco::hicann_dls::vx::SynapseRowOnSynapseDriver::top] << "), bottom("
	   << config.m_row_modes[halco::hicann_dls::vx::SynapseRowOnSynapseDriver::bottom] << ")})";
	return os;
}

} // namespace grenade::vx::vertex
