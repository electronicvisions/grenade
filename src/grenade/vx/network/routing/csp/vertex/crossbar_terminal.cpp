#include "grenade/vx/network/routing/csp/vertex/crossbar_terminal.h"

namespace grenade::vx::network::routing::csp {

CrossbarTerminal::~CrossbarTerminal() {}

std::string CrossbarTerminal::get_name() const
{
	return "CrossbarTerminal";
}

void CrossbarTerminal::update(Gecode::Space&, RoutingVertex&) {}

Gecode::IntVarArgs CrossbarTerminal::get_parameters() const
{
	Gecode::IntVarArgs ret;
	return ret;
}

std::vector<std::string> CrossbarTerminal::get_parameter_names() const
{
	std::vector<std::string> ret;
	return ret;
}


std::unique_ptr<RoutingVertex> CrossbarTerminal::copy() const
{
	return std::make_unique<CrossbarTerminal>(*this);
}

std::unique_ptr<RoutingVertex> CrossbarTerminal::move()
{
	return std::make_unique<CrossbarTerminal>(std::move(*this));
}

std::vector<size_t> CrossbarTerminal::get_parameter_bitcounts() const
{
	return std::vector<size_t>{};
}

bool CrossbarTerminal::is_equal_to(RoutingVertex const&) const
{
	return true;
}

} // namespace grenade::vx::network::routing::csp