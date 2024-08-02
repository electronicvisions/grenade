#include "grenade/vx/network/plasticity_rule.h"

#include "hate/indent.h"
#include <ostream>
#include <sstream>

namespace grenade::vx::network {

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
	for (auto const& neuron : population.neuron_readout_sources) {
		os << "\t\t";
		for (auto const& [compartment, denmems] : neuron) {
			os << compartment << ": "
			   << "\n";
			for (auto const& denmem : denmems) {
				os << "\t\t\t";
				if (denmem) {
					os << *denmem;
				} else {
					os << "unset";
				}
				os << "\n";
			}
		}
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
	       recording == other.recording && id == other.id;
}

bool PlasticityRule::operator!=(PlasticityRule const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule const& plasticity_rule)
{
	hate::IndentingOstream ios(os);
	ios << "PlasticityRule(\n";
	ios << hate::Indentation("\t") << "projections:\n" << hate::Indentation("\t\t");
	for (auto const& d : plasticity_rule.projections) {
		ios << d << "\n";
	}
	ios << hate::Indentation("\t") << "populations:\n" << hate::Indentation("\t\t");
	for (auto const& d : plasticity_rule.populations) {
		ios << d << "\n";
	}
	ios << hate::Indentation("\t") << "kernel:\n"
	    << hate::Indentation("\t\t") << plasticity_rule.kernel << "\n";
	ios << hate::Indentation("\t") << plasticity_rule.timer << "\n";
	ios << "enable_requires_one_source_per_row_in_order: " << std::boolalpha
	    << plasticity_rule.enable_requires_one_source_per_row_in_order << "\n";
	ios << "recording: ";
	if (plasticity_rule.recording) {
		std::visit([&](auto const& recording) { ios << recording; }, *plasticity_rule.recording);
	} else {
		ios << "disabled";
	}
	ios << "\n" << plasticity_rule.id;
	ios << "\n" << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network
