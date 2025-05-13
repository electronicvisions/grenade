#include "grenade/vx/network/external_source_population.h"

#include "hate/indent.h"
#include "hate/join.h"
#include <ostream>
#include <sstream>

namespace grenade::vx::network {

ExternalSourcePopulation::Neuron::Neuron(bool enable_record_spikes) :
    enable_record_spikes(enable_record_spikes)
{}

std::ostream& operator<<(std::ostream& os, ExternalSourcePopulation::Neuron const& value)
{
	return os << "Neuron(" << value.enable_record_spikes << ")";
}

ExternalSourcePopulation::ExternalSourcePopulation(
    std::vector<Neuron> neurons, common::EntityOnChip::ChipOnExecutor const chip_on_executor) :
    common::EntityOnChip(chip_on_executor), neurons(std::move(neurons))
{}

bool ExternalSourcePopulation::operator==(ExternalSourcePopulation const& other) const
{
	return neurons == other.neurons && static_cast<common::EntityOnChip const&>(*this) ==
	                                       static_cast<common::EntityOnChip const&>(other);
}

bool ExternalSourcePopulation::operator!=(ExternalSourcePopulation const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, ExternalSourcePopulation const& population)
{
	hate::IndentingOstream ios(os);
	ios << "ExternalSourcePopulation(\n";
	ios << hate::Indentation("\t");
	ios << static_cast<common::EntityOnChip const&>(population) << "\n";
	ios << hate::join(population.neurons, "\n") << "\n";
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network
