#include "grenade/vx/vertex/synapse_driver.h"

#include "grenade/cerealization.h"
#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/padi_bus.h"
#include "halco/common/cerealization_geometry.h"
#include "halco/common/cerealization_typed_array.h"
#include <ostream>

namespace grenade::vx::vertex {

bool SynapseDriver::Config::operator==(SynapseDriver::Config const& other) const
{
	return (row_address_compare_mask == other.row_address_compare_mask) &&
	       (row_modes == other.row_modes) && (enable_address_out == other.enable_address_out);
}

bool SynapseDriver::Config::operator!=(SynapseDriver::Config const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void SynapseDriver::Config::serialize(Archive& ar, std::uint32_t const)
{
	ar(row_address_compare_mask);
	ar(row_modes);
	ar(enable_address_out);
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
	return halco::hicann_dls::vx::v2::PADIBusOnDLS(
	           m_coordinate.toSynapseDriverOnSynapseDriverBlock().toPADIBusOnPADIBusBlock(),
	           m_coordinate.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS()) ==
	       input.get_coordinate();
}

std::ostream& operator<<(std::ostream& os, SynapseDriver const& config)
{
	os << "SynapseDriver(coordinate: " << config.m_coordinate
	   << ", row_mask: " << config.m_config.row_address_compare_mask << ", row_modes: {top("
	   << config.m_config.row_modes[halco::hicann_dls::vx::v2::SynapseRowOnSynapseDriver::top]
	   << "), bottom("
	   << config.m_config.row_modes[halco::hicann_dls::vx::v2::SynapseRowOnSynapseDriver::bottom]
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

template <typename Archive>
void SynapseDriver::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_coordinate);
	ar(m_config);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::SynapseDriver)
CEREAL_CLASS_VERSION(grenade::vx::vertex::SynapseDriver, 1)
