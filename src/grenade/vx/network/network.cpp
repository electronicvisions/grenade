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
	hate::IndentingOstream ios(os);
	ios << "ExecutionInstance(\n";
	ios << hate::Indentation("\t");
	ios << "populations:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& [descriptor, population] : execution_instance.populations) {
		ios << descriptor << ": ";
		std::visit([&ios](auto const& pop) { ios << pop; }, population);
	}
	ios << hate::Indentation("\t") << "projections:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& [descriptor, projection] : execution_instance.projections) {
		ios << descriptor << ": " << projection << "\n";
	}
	ios << hate::Indentation("\t") << "plasticity rules:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& [descriptor, rule] : execution_instance.plasticity_rules) {
		ios << descriptor << ": " << rule << "\n";
	}
	ios << hate::Indentation() << "\n)";
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
	hate::IndentingOstream ios(os);
	ios << "Network(\n" << hate::Indentation("\t");
	for (auto [id, execution_instance] : network.execution_instances) {
		ios << id << ":\n";
		ios << execution_instance;
		ios << "\n";
	}
	for (auto const& [id, projection] : network.inter_execution_instance_projections) {
		ios << id << ":\n";
		ios << projection;
		ios << "\n";
	}
	ios << hate::Indentation() << ")";
	return os;
}

} // namespace grenade::vx::network
