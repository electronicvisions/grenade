#include "grenade/vx/signal_flow/vertex/background_spike_source.h"

#include "grenade/vx/signal_flow/vertex/external_input.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

BackgroundSpikeSource::BackgroundSpikeSource(
    Config const& config, Coordinate const& coordinate, ChipOnExecutor const& chip_on_executor) :
    EntityOnChip(chip_on_executor), m_config(config), m_coordinate(coordinate)
{}

std::ostream& operator<<(std::ostream& os, BackgroundSpikeSource const& config)
{
	os << "BackgroundSpikeSource(\n" << config.m_config << "\n" << config.m_coordinate << "\n)";
	return os;
}

bool BackgroundSpikeSource::operator==(BackgroundSpikeSource const& other) const
{
	return m_config == other.m_config && m_coordinate == other.m_coordinate;
}

bool BackgroundSpikeSource::operator!=(BackgroundSpikeSource const& other) const
{
	return !(*this == other);
}

BackgroundSpikeSource::Config const& BackgroundSpikeSource::get_config() const
{
	return m_config;
}

BackgroundSpikeSource::Coordinate const& BackgroundSpikeSource::get_coordinate() const
{
	return m_coordinate;
}

} // namespace grenade::vx::signal_flow::vertex
