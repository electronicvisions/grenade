#include "grenade/vx/signal_flow/vertex/spikeio_source_population.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

SpikeIOSourcePopulation::SpikeIOSourcePopulation(
    Config const& config,
    OutputRouting const& out_rt,
    InputRouting const& in_rt,
    ChipOnExecutor const& chip_on_executor) :
    EntityOnChip(chip_on_executor), m_config(config), m_output(out_rt), m_input(in_rt)
{
}

Port SpikeIOSourcePopulation::output() const
{
	return Port(1, ConnectionType::TimedSpikeToChipSequence);
}

SpikeIOSourcePopulation::Config const& SpikeIOSourcePopulation::get_config() const
{
	return m_config;
}
SpikeIOSourcePopulation::OutputRouting const& SpikeIOSourcePopulation::get_output_routing() const
{
	return m_output;
}
SpikeIOSourcePopulation::InputRouting const& SpikeIOSourcePopulation::get_input_routing() const
{
	return m_input;
}

std::ostream& operator<<(std::ostream& os, SpikeIOSourcePopulation const& v)
{
	os << "SpikeIOSourcePopulation(" << static_cast<common::EntityOnChip const&>(v) << ",\n"
	   << v.m_config << "\n";
	os << "\toutput_routing: " << v.m_output.size() << " entries\n";
	os << "\tinput_routing: " << v.m_input.size() << " entries\n)";
	return os;
}

bool SpikeIOSourcePopulation::operator==(SpikeIOSourcePopulation const& other) const
{
	return (m_config == other.m_config) && (m_output == other.m_output) &&
	       (m_input == other.m_input);
}
bool SpikeIOSourcePopulation::operator!=(SpikeIOSourcePopulation const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow::vertex
