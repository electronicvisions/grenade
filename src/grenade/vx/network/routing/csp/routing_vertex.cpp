#include "grenade/vx/network/routing/csp/routing_vertex.h"
#include "grenade/vx/network/routing/csp/helper/bitset_beautifier.h"
#include "hate/indent.h"

namespace grenade::vx::network::routing::csp {

RoutingVertex::~RoutingVertex() {}

void RoutingVertex::trace(Gecode::Space& space)
{
	auto const parameters = get_parameters();
	auto const parameter_names = get_parameter_names();
	if (static_cast<size_t>(parameters.size()) != parameter_names.size()) {
		throw std::logic_error("parameters and parameter names size doesn't match.");
	}
	auto const name = get_name();
	m_tracers.resize(parameters.size());
	for (size_t i = 0; i < m_tracers.size(); ++i) {
		m_tracers.at(i) = std::make_shared<IntTracer>(name + "." + parameter_names.at(i));
		Gecode::IntVarArgs tmp;
		tmp << parameters[i];
		Gecode::trace(
		    space, tmp, (Gecode::TE_PRUNE | Gecode::TE_FIX | Gecode::TE_FAIL | Gecode::TE_DONE),
		    *m_tracers.at(i));
	}
}

std::ostream& RoutingVertex::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	BitsetBeautifier beautifier;

	ios << get_name() << "(";

	ios << hate::Indentation("\t");
	auto parameters = get_parameters();
	auto parameter_names = get_parameter_names();
	auto parameter_bitcounts = get_parameter_bitcounts();
	assert(static_cast<size_t>(parameters.size()) == parameter_names.size());
	assert(static_cast<size_t>(parameters.size()) == parameter_bitcounts.size());

	for (size_t i = 0; i < parameter_names.size(); i++) {
		ios << parameter_names.at(i) << ": ";
		if (parameters[i].assigned()) {
			ios << beautifier(parameters[i].val(), parameter_bitcounts.at(i)) << "("
			    << parameters[i] << ")";
		} else {
			ios << parameters[i];
		}
		ios << "\n";
	}

	ios << hate::Indentation("");
	ios << ")";
	return os;
}

} // namespace grenade::vx::network::routing::csp