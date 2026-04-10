#include "grenade/vx/network/routing/csp/vertex/target.h"

namespace grenade::vx::network::routing::csp {

Target::~Target() {}

std::string Target::get_name() const
{
	return "Target";
}

void Target::update(Gecode::Space&, RoutingVertex&) {}

Gecode::IntVarArgs Target::get_parameters() const
{
	return Gecode::IntVarArgs();
}

std::vector<std::string> Target::get_parameter_names() const
{
	return {};
}

std::unique_ptr<RoutingVertex> Target::copy() const
{
	return std::make_unique<Target>(*this);
}

std::unique_ptr<RoutingVertex> Target::move()
{
	return std::make_unique<Target>(std::move(*this));
}

std::vector<size_t> Target::get_parameter_bitcounts() const
{
	return std::vector<size_t>{};
}

bool Target::is_equal_to(RoutingVertex const&) const
{
	return false;
}

} // namespace grenade::vx::network::routing::csp