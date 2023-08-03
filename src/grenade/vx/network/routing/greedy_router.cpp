#include "grenade/vx/network/routing/greedy_router.h"

#include "grenade/vx/network/build_connection_routing.h"
#include "grenade/vx/network/routing/greedy/routing_builder.h"
#include "hate/timer.h"
#include <chrono>
#include <iostream>
#include <ostream>
#include <stdexcept>

namespace grenade::vx::network::routing {

GreedyRouter::Options::Options() {}

std::ostream& operator<<(std::ostream& os, GreedyRouter::Options const& options)
{
	os << "Options(\n";
	os << "\tsynapse_driver_allocation_policy: " << options.synapse_driver_allocation_policy
	   << "\n";
	os << "\tsynapse_driver_allocation_timeout: "
	   << (options.synapse_driver_allocation_timeout
	           ? hate::to_string(*options.synapse_driver_allocation_timeout)
	           : "infinite");
	os << "\n)";
	return os;
}


struct GreedyRouter::Impl
{
	Impl(GreedyRouter::Options const& options) : m_options(options), m_builder() {}

	GreedyRouter::Options m_options;
	routing::greedy::RoutingBuilder m_builder;
};


GreedyRouter::GreedyRouter(Options const& options) : m_impl(std::make_unique<Impl>(options)) {}

GreedyRouter::~GreedyRouter() {}

RoutingResult GreedyRouter::operator()(std::shared_ptr<Network> const& network)
{
	if (!m_impl) {
		throw std::logic_error("Unexpected access to moved-from object.");
	}

	if (!network) {
		throw std::runtime_error("Routing only possible for non-null network.");
	}

	hate::Timer timer;
	RoutingResult result;
	for (auto const& [id, execution_instance] : network->execution_instances) {
		auto const connection_routing_result = build_connection_routing(execution_instance);
		result.execution_instances.emplace(
		    id, m_impl->m_builder.route(
		            execution_instance, connection_routing_result, m_impl->m_options));
	}
	result.timing_statistics.routing += std::chrono::microseconds(timer.get_us());
	return result;
}

} // namespace grenade::vx::network::routing
