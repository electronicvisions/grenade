#include "grenade/vx/network/background_source_population.h"

#include "hate/indent.h"
#include "hate/join.h"
#include <ostream>
#include <sstream>

namespace grenade::vx::network {

BackgroundSourcePopulation::Neuron::Neuron(bool enable_record_spikes) :
    enable_record_spikes(enable_record_spikes)
{}

std::ostream& operator<<(std::ostream& os, BackgroundSourcePopulation::Neuron const& value)
{
	return os << "Neuron(" << value.enable_record_spikes << ")";
}

BackgroundSourcePopulation::BackgroundSourcePopulation(
    std::vector<Neuron> neurons,
    Coordinate const& coordinate,
    Config const& config,
    common::EntityOnChip::ChipOnExecutor const chip_on_executor) :
    common::EntityOnChip(chip_on_executor),
    neurons(std::move(neurons)),
    coordinate(coordinate),
    config(config)
{}

bool BackgroundSourcePopulation::Config::operator==(
    BackgroundSourcePopulation::Config const& other) const
{
	return period == other.period && rate == other.rate && seed == other.seed && enable_random &&
	       other.enable_random;
}

bool BackgroundSourcePopulation::Config::operator!=(
    BackgroundSourcePopulation::Config const& other) const
{
	return !(*this == other);
}

bool BackgroundSourcePopulation::operator==(BackgroundSourcePopulation const& other) const
{
	return neurons == other.neurons && coordinate == other.coordinate && config == other.config &&
	       static_cast<common::EntityOnChip const&>(*this) ==
	           static_cast<common::EntityOnChip const&>(other);
}

bool BackgroundSourcePopulation::operator!=(BackgroundSourcePopulation const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, BackgroundSourcePopulation::Config const& config)
{
	os << "Config(" << config.period << ", " << config.rate << ", " << config.seed
	   << ", enable_random: " << std::boolalpha << config.enable_random << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, BackgroundSourcePopulation const& population)
{
	hate::IndentingOstream ios(os);
	ios << "BackgroundSourcePopulation(\n";
	ios << hate::Indentation("\t");
	ios << static_cast<common::EntityOnChip const&>(population) << "\n";
	ios << hate::join(population.neurons, "\n") << "\n";
	ios << "coordinate:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& [hemisphere, padi_bus] : population.coordinate) {
		ios << halco::hicann_dls::vx::v3::PADIBusOnDLS(padi_bus, hemisphere.toPADIBusBlockOnDLS())
		    << "\n";
	}
	ios << hate::Indentation("\t");
	ios << population.config << "\n";
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network
