#include "grenade/vx/signal_flow/vertex/synapse_driver.h"

#include "grenade/vx/signal_flow/port_restriction.h"
#include "grenade/vx/signal_flow/vertex/padi_bus.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

bool SynapseDriver::Config::operator==(SynapseDriver::Config const& other) const
{
	return (row_address_compare_mask == other.row_address_compare_mask) &&
	       (row_modes == other.row_modes) && (enable_address_out == other.enable_address_out);
}

bool SynapseDriver::Config::operator!=(SynapseDriver::Config const& other) const
{
	return !(*this == other);
}

SynapseDriver::SynapseDriver(Coordinate const& coordinate, Config const& config) :
    m_coordinate(coordinate), m_config(config)
{}

SynapseDriver::Coordinate SynapseDriver::get_coordinate() const
{
	return m_coordinate;
}

SynapseDriver::Config SynapseDriver::get_config() const
{
	return m_config;
}

bool SynapseDriver::supports_input_from(
    PADIBus const& input, std::optional<PortRestriction> const&) const
{
	return halco::hicann_dls::vx::v3::PADIBusOnDLS(
	           m_coordinate.toSynapseDriverOnSynapseDriverBlock().toPADIBusOnPADIBusBlock(),
	           m_coordinate.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS()) ==
	       input.get_coordinate();
}

std::ostream& operator<<(std::ostream& os, SynapseDriver const& config)
{
	os << "SynapseDriver(coordinate: " << config.m_coordinate
	   << ", row_mask: " << config.m_config.row_address_compare_mask << ", row_modes: {top("
	   << config.m_config.row_modes[halco::hicann_dls::vx::v3::SynapseRowOnSynapseDriver::top]
	   << "), bottom("
	   << config.m_config.row_modes[halco::hicann_dls::vx::v3::SynapseRowOnSynapseDriver::bottom]
	   << ")}, enable_address_out: " << config.m_config.enable_address_out << ")";
	return os;
}

bool SynapseDriver::operator==(SynapseDriver const& other) const
{
	return (m_coordinate == other.m_coordinate) && (m_config == other.m_config);
}

bool SynapseDriver::operator!=(SynapseDriver const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow::vertex
