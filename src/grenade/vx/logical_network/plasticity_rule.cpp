#include "grenade/vx/logical_network/plasticity_rule.h"

#include "hate/indent.h"
#include <ostream>
#include <sstream>

namespace grenade::vx::logical_network {

bool PlasticityRule::operator==(PlasticityRule const& other) const
{
	return projections == other.projections && kernel == other.kernel && timer == other.timer &&
	       enable_requires_one_source_per_row_in_order ==
	           other.enable_requires_one_source_per_row_in_order;
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
	os << "\tkernel: \n" << hate::indent(plasticity_rule.kernel, "\t\t") << "\n";
	os << "\t" << plasticity_rule.timer << "\n";
	std::stringstream ss;
	ss << "\tenable_requires_one_source_per_row_in_order: " << std::boolalpha
	   << plasticity_rule.enable_requires_one_source_per_row_in_order << "\n";
	os << ss.str();
	os << ")";
	return os;
}

} // namespace grenade::vx::logical_network
