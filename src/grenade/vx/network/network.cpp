#include "grenade/vx/network/network.h"

#include "hate/indent.h"
#include "hate/join.h"
#include <ostream>
#include <sstream>

namespace grenade::vx::network {

bool Network::ExecutionInstance::operator==(Network::ExecutionInstance const& other) const
{
	return populations == other.populations && projections == other.projections &&
	       madc_recording == other.madc_recording && cadc_recording == other.cadc_recording &&
	       pad_recording == other.pad_recording && plasticity_rules == other.plasticity_rules;
}

bool Network::ExecutionInstance::operator!=(Network::ExecutionInstance const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, Network::ExecutionInstance const& execution_instance)
{
	os << "ExecutionInstance(\n";
	os << "\tpopulations:\n";
	{
		std::vector<std::string> pops;
		for (auto const& [descriptor, population] : execution_instance.populations) {
			std::stringstream ss;
			ss << descriptor << ": ";
			std::visit([&ss](auto const& pop) { ss << pop; }, population);
			pops.push_back(ss.str());
		}
		os << hate::indent(hate::join_string(pops.begin(), pops.end(), "\n"), "\t\t");
		os << "\tprojections:\n";
	}
	{
		std::stringstream ss;
		for (auto const& [descriptor, projection] : execution_instance.projections) {
			ss << descriptor << ": " << projection << "\n";
		}
		os << hate::indent(ss.str(), "\t\t");
		os << "\tplasticity rules:\n";
	}
	{
		std::stringstream ss;
		for (auto const& [descriptor, rule] : execution_instance.plasticity_rules) {
			ss << descriptor << ": " << rule << "\n";
		}
		os << hate::indent(ss.str(), "\t\t");
	}
	os << "\n)";
	return os;
}


bool Network::operator==(Network const& other) const
{
	return execution_instances == other.execution_instances &&
	       inter_execution_instance_projections == other.inter_execution_instance_projections &&
	       topologically_sorted_execution_instance_ids ==
	           other.topologically_sorted_execution_instance_ids;
}

bool Network::operator!=(Network const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, Network const& network)
{
	os << "Network(\n";
	for (auto [id, execution_instance] : network.execution_instances) {
		os << "\t" << id << ":\n";
		std::stringstream ss;
		ss << execution_instance;
		os << hate::indent(ss.str(), "\t") << "\n";
	}
	for (auto const& [id, projection] : network.inter_execution_instance_projections) {
		os << "\t" << id << ":\n";
		std::stringstream ss;
		ss << projection;
		os << hate::indent(ss.str(), "\t") << "\n";
	}
	os << ")";
	return os;
}

} // namespace grenade::vx::network
