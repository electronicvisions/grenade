#include "grenade/vx/network/placed_atomic/plasticity_rule.h"

#include "hate/indent.h"
#include <ostream>
#include <sstream>

namespace grenade::vx::network::placed_atomic {

bool PlasticityRule::Timer::operator==(PlasticityRule::Timer const& other) const
{
	return start == other.start && period == other.period && num_periods == other.num_periods;
}

bool PlasticityRule::Timer::operator!=(PlasticityRule::Timer const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::Timer const& timer)
{
	os << "Timer(start: " << timer.start << ", period: " << timer.period
	   << ", num_periods: " << timer.num_periods << ")";
	return os;
}

bool PlasticityRule::PopulationHandle::operator==(
    PlasticityRule::PopulationHandle const& other) const
{
	return descriptor == other.descriptor && neuron_readout_sources == other.neuron_readout_sources;
}

bool PlasticityRule::PopulationHandle::operator!=(
    PlasticityRule::PopulationHandle const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::PopulationHandle const& population)
{
	os << "PopulationHandle(\n";
	os << "\tdescriptor: " << population.descriptor << "\n";
	os << "\tneuron_readout_sources:\n";
	for (auto const& readout_source : population.neuron_readout_sources) {
		os << "\t\t";
		if (readout_source) {
			os << *readout_source;
		} else {
			os << "unset";
		}
		os << "\n";
	}
	os << ")";
	return os;
}

bool PlasticityRule::operator==(PlasticityRule const& other) const
{
	return projections == other.projections && populations == other.populations &&
	       kernel == other.kernel && timer == other.timer &&
	       enable_requires_one_source_per_row_in_order ==
	           other.enable_requires_one_source_per_row_in_order &&
	       recording == other.recording;
}

bool PlasticityRule::operator!=(PlasticityRule const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule const& plasticity_rule)
{
	os << "PlasticityRule(\n";
	os << "\tprojections:\n";
	for (auto const& d : plasticity_rule.projections) {
		os << "\t\t" << d << "\n";
	}
	os << "\tpopulations:\n";
	for (auto const& d : plasticity_rule.populations) {
		std::stringstream ss;
		ss << d;
		os << hate::indent(ss.str(), "\t\t") << "\n";
	}
	os << "\tkernel: \n" << hate::indent(plasticity_rule.kernel, "\t\t") << "\n";
	os << "\t" << plasticity_rule.timer << "\n";
	std::stringstream ss;
	ss << "\tenable_requires_one_source_per_row_in_order: " << std::boolalpha
	   << plasticity_rule.enable_requires_one_source_per_row_in_order << "\n";
	os << ss.str();
	os << "\trecording: ";
	if (plasticity_rule.recording) {
		std::visit([&](auto const& recording) { os << recording; }, *plasticity_rule.recording);
	} else {
		os << "disabled";
	}
	os << "\n";
	os << ")";
	return os;
}

} // namespace grenade::vx::network::placed_atomic
