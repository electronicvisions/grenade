#include "grenade/vx/network/routing/greedy_router.h"

#include "grenade/common/partitioned_vertex.h"
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

RoutingResult GreedyRouter::operator()(grenade::common::LinkedTopology const& topology)
{
	if (!m_impl) {
		throw std::logic_error("Unexpected access to moved-from object.");
	}

	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::vector<grenade::common::VertexOnTopology>>
	    partitioned_vertices_per_execution_instance;

	for (auto const& vertex_descriptor : topology.get_reference().vertices()) {
		auto const& partitioned_vertex = dynamic_cast<grenade::common::PartitionedVertex const&>(
		    topology.get_reference().get(vertex_descriptor));
		if (partitioned_vertex.get_time_domain()) {
			partitioned_vertices_per_execution_instance
			    [partitioned_vertex.get_execution_instance_on_executor().value()]
			        .push_back(vertex_descriptor);
		}
	}

	hate::Timer timer;
	RoutingResult result;
	for (auto const& [id, partitioned_vertex_descriptors] :
	     partitioned_vertices_per_execution_instance) {
		auto const connection_routing_result =
		    build_connection_routing(topology, partitioned_vertex_descriptors);
		result.execution_instances.emplace(
		    id, m_impl->m_builder.route(
		            topology, partitioned_vertex_descriptors, connection_routing_result,
		            m_impl->m_options));
	}
	result.timing_statistics.routing += std::chrono::microseconds(timer.get_us());
	return result;
}

} // namespace grenade::vx::network::routing
