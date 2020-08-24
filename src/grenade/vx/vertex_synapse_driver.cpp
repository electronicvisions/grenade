#include "grenade/vx/vertex/synapse_driver.h"

#include "grenade/cerealization.h"
#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/padi_bus.h"
#include "halco/common/cerealization_geometry.h"
#include "halco/common/cerealization_typed_array.h"

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
	os << "SynapseDriver(coordinate: " << config.m_coordinate << ", row_mask: " << config.m_config
	   << ", row_modes: {top("
	   << config.m_row_modes[halco::hicann_dls::vx::v2::SynapseRowOnSynapseDriver::top]
	   << "), bottom("
	   << config.m_row_modes[halco::hicann_dls::vx::v2::SynapseRowOnSynapseDriver::bottom] << ")})";
	return os;
}

bool SynapseDriver::operator==(SynapseDriver const& other) const
{
	return (m_coordinate == other.m_coordinate) && (m_config == other.m_config) &&
	       (m_row_modes == other.m_row_modes);
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
	ar(m_row_modes);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::SynapseDriver)
CEREAL_CLASS_VERSION(grenade::vx::vertex::SynapseDriver, 0)
