#include "grenade/vx/network/routing/csp/router_space_factory.h"

#include "grenade/vx/network/routing/csp/vertex/filter.h"
#include "grenade/vx/network/routing/csp/vertex/source.h"
#include "hate/timer.h"

namespace grenade::vx::network::routing::csp {

RouterSpaceFactory::RouterSpaceFactory() : m_router_space(std::make_unique<RouterSpace>()) {}

std::unique_ptr<RouterSpace> RouterSpaceFactory::done()
{
	auto new_router_space = std::make_unique<RouterSpace>();
	std::swap(m_router_space, new_router_space);

	return new_router_space;
}

void RouterSpaceFactory::build_constraints(
    std::map<SourceTargetPair, std::map<size_t, size_t>> active_routes)
{
	auto [routes, routes_cyclic] = build_routes();
	build_label_constraints(active_routes);
	build_route_constraints(routes, routes_cyclic, active_routes);
	build_filter_constraints(routes, routes_cyclic);

	build_branching(routes, routes_cyclic);
	build_tracing();
}

RouterSpace& RouterSpaceFactory::get_space()
{
	if (!m_router_space) {
		throw std::runtime_error("No router space in factory.");
	}
	return *m_router_space;
}

RoutingGraph& RouterSpaceFactory::get_graph()
{
	return get_space().m_routing_graph;
}

std::pair<RouteCollector::Routes, RouteCollector::Routes> RouterSpaceFactory::build_routes()
{
	hate::Timer timer;
	RouteCollector route_collector;
	auto [routes_acyclic, routes_cyclic] = route_collector(m_router_space->m_routing_graph);

	size_t num_routes = 0;
	for (auto const& [route_limits, routes] : routes_acyclic) {
		LOG4CXX_TRACE(
		    m_router_space->m_shared_storage->logger,
		    "Found " << routes.size() << " routes for " << route_limits);
		num_routes += routes.size();
	}

	LOG4CXX_DEBUG(
	    m_router_space->m_shared_storage->logger,
	    "Created " << num_routes << " routes in " << timer.print() << ".");

	return {routes_acyclic, routes_cyclic};
}

void RouterSpaceFactory::build_label_constraints(
    std::map<SourceTargetPair, std::map<size_t, size_t>> const& active_routes)
{
	// Enforce distinct source labels
	{
		hate::Timer timer;

		std::set<grenade::common::VertexOnTopology> sources;
		for (auto const& [route_limits, _] : m_router_space->m_route_activities) {
			Gecode::IntVarArgs labels;

			if (sources.contains(route_limits.source)) {
				continue;
			}
			sources.insert(route_limits.source);

			auto const& source = dynamic_cast<Source const&>(
			    m_router_space->m_routing_graph.get(route_limits.source));
			for (size_t source_index = 0; source_index < source.size(); source_index++) {
				labels << source.get_label(source_index);
			}
			LOG4CXX_TRACE(
			    m_router_space->m_shared_storage->logger,
			    source.size() << " source labels constrained to be unique for "
			                  << route_limits.source);
			Gecode::distinct(*m_router_space, labels);
		}

		LOG4CXX_DEBUG(
		    m_router_space->m_shared_storage->logger,
		    "Constrained source labels to be unique in " << timer.print() << ".");
	}

	// Enforce distinct or equal target labels
	{
		hate::Timer timer;

		// Collect all target nodes
		std::map<
		    grenade::common::VertexOnTopology,
		    std::map<size_t, std::set<std::pair<grenade::common::VertexOnTopology, size_t>>>>
		    source_indices_per_target_index;

		for (auto const& [route_limits, index_mapping] : active_routes) {
			for (auto const& [source_index, target_index] : index_mapping) {
				source_indices_per_target_index[route_limits.target][target_index].insert(
				    {route_limits.source, source_index});
			}
		}

		for (auto const& [target, source_indices_per_target] : source_indices_per_target_index) {
			Gecode::IntVarArgs distinct_labels;
			for (auto const& [target_index, source_indices] : source_indices_per_target) {
				// Enforce distinct labels for all source labels projecting on a different
				// target_index.

				for (auto const& [source_descriptor, source_index] : source_indices) {
					auto const& source = dynamic_cast<Source const&>(
					    m_router_space->m_routing_graph.get(source_descriptor));
					auto label = source.get_label(source_index);
					distinct_labels << label;
				}
			}
			Gecode::distinct(*m_router_space, distinct_labels);
		}

		LOG4CXX_DEBUG(
		    m_router_space->m_shared_storage->logger,
		    "Constrained target labels to be unique in " << timer.print() << ".");
	}

	// Check that the amount of available source labels is sufficient to satisify the requests
	// source-target-pair connections
	{
		std::map<grenade::common::VertexOnTopology, std::set<size_t>> requested_indices_per_source;
		for (auto const& [route_limits, source_indices] : active_routes) {
			for (auto const& [source_index, _] : source_indices) {
				requested_indices_per_source[route_limits.source].insert(source_index);
			}
		}

		for (auto const& [source_descriptor, requested_indices] : requested_indices_per_source) {
			auto const& source =
			    dynamic_cast<Source const&>(m_router_space->m_routing_graph.get(source_descriptor));
			if (requested_indices.size() > source.size()) {
				throw std::range_error(
				    "Requiring more connections from a source than indices available.");
			}
		}
	}
}

void RouterSpaceFactory::build_route_constraints(
    RouteCollector::Routes& routes_acyclic,
    RouteCollector::Routes& routes_cyclic,
    std::map<SourceTargetPair, std::map<size_t, size_t>> const& active_routes)
{
	// Create route constraints for acyclic routes
	{
		hate::Timer timer;
		for (auto const& [route_limits, routes] : routes_acyclic) {
			auto const& source = dynamic_cast<Source const&>(
			    m_router_space->m_routing_graph.get(route_limits.source));

			auto& local_route_activities = m_router_space->m_route_activities[route_limits];
			local_route_activities.resize(source.size());

			// For each source index have array over different routes
			for (size_t source_index = 0; source_index < source.size(); source_index++) {
				local_route_activities.at(source_index) =
				    Gecode::BoolVarArray(*m_router_space, routes.size(), 0, 1);
			}
		}

		size_t num_route_activities = 0;
		for (auto const& [_, route_activities] : m_router_space->m_route_activities) {
			for (auto const& route_activity_per_source_label : route_activities) {
				num_route_activities += route_activity_per_source_label.size();
			}
		}

		LOG4CXX_DEBUG(
		    m_router_space->m_shared_storage->logger,
		    "Created " << num_route_activities << " acyclic route activity values in "
		               << timer.print() << ".");
	}

	// Constrain acyclic routes to one active route per source target connection
	{
		for (auto const& [route_limits, routes_per_source_index] :
		     m_router_space->m_route_activities) {
			for (size_t source_index = 0; source_index < routes_per_source_index.size();
			     ++source_index) {
				if ((active_routes.contains(route_limits) &&
				     active_routes.at(route_limits).contains(source_index))) {
					// Enable exaclty one route
					Gecode::linear(
					    *m_router_space, routes_per_source_index.at(source_index), Gecode::IRT_EQ,
					    1);
				} else {
					// Disable all routes
					Gecode::linear(
					    *m_router_space, routes_per_source_index.at(source_index), Gecode::IRT_EQ,
					    0);
				}
			}
		}
	}

	// Create route constraints for cyclic routes
	{
		hate::Timer timer;
		for (auto const& [route_limits, routes] : routes_cyclic) {
			auto const& source = dynamic_cast<Source const&>(
			    m_router_space->m_routing_graph.get(route_limits.source));

			auto& local_route_activities = m_router_space->m_cyclic_route_activities[route_limits];
			local_route_activities.resize(source.size());

			// For each source index have array over different routes
			for (size_t source_index = 0; source_index < source.size(); source_index++) {
				local_route_activities.at(source_index) =
				    Gecode::BoolVarArray(*m_router_space, routes.size(), 0, 1);
			}
		}

		size_t num_route_activities = 0;
		for (auto const& [_, route_activities] : m_router_space->m_cyclic_route_activities) {
			for (auto const& route_activity_per_source_label : route_activities) {
				num_route_activities += route_activity_per_source_label.size();
			}
		}

		LOG4CXX_DEBUG(
		    m_router_space->m_shared_storage->logger,
		    "Created " << num_route_activities << " cyclic route activity values in "
		               << timer.print() << ".");
	}

	// Constrain cyclic routes to zero active routes per source target connection
	{
		for (auto const& [route_limits, routes_per_source_index] :
		     m_router_space->m_cyclic_route_activities) {
			for (size_t source_index = 0; source_index < routes_per_source_index.size();
			     ++source_index) {
				// Disable all routes
				Gecode::linear(
				    *m_router_space, routes_per_source_index.at(source_index), Gecode::IRT_EQ, 0);
			}
		}
	}
}

void RouterSpaceFactory::build_filter_constraints(
    RouteCollector::Routes& routes_acyclic, RouteCollector::Routes&)
{
	// Concatenate the filters on each route.
	{
		hate::Timer timer;
		std::map<
		    grenade::common::VertexOnTopology,
		    std::map<size_t, std::map<grenade::common::VertexOnTopology, Gecode::BoolVar>>>
		    filter_results;
		for (auto const& [route_limits, routes] : routes_acyclic) {
			auto const& source = dynamic_cast<Source const&>(
			    m_router_space->m_routing_graph.get(route_limits.source));

			auto& local_route_activities = m_router_space->m_route_activities[route_limits];
			assert(local_route_activities.size() == source.size());

			auto& local_filter_results = filter_results[route_limits.source];

			for (size_t source_index = 0; source_index < local_route_activities.size();
			     source_index++) {
				auto& local_route_activities_per_label = local_route_activities.at(source_index);
				auto source_label = source.get_label(source_index);
				auto& local_filter_results_per_label = local_filter_results[source_index];

				for (size_t i = 0; i < routes.size(); ++i) {
					Gecode::BoolVarArgs tmp;
					for (auto const& filter_node : routes.at(i).descriptors) {
						if (!local_filter_results_per_label.contains(filter_node)) {
							if (auto* filter_vertex = dynamic_cast<Filter*>(
							        &(m_router_space->m_routing_graph.get_mutable(filter_node)));
							    filter_vertex) {
								auto filter_result =
								    filter_vertex->apply_filter(*m_router_space, source_label);

								local_filter_results_per_label.emplace(filter_node, filter_result);
								tmp << filter_result;
							}

						} else {
							tmp << local_filter_results_per_label.at(filter_node);
						}
					}
					Gecode::rel(
					    *m_router_space, Gecode::BOT_AND, tmp, local_route_activities_per_label[i]);
				}
			}
		}
		size_t num_route_activities = 0;
		for (auto const& [_, route_activities] : m_router_space->m_route_activities) {
			for (auto const& route_activity_per_source_label : route_activities) {
				num_route_activities += route_activity_per_source_label.size();
			}
		}

		LOG4CXX_DEBUG(
		    m_router_space->m_shared_storage->logger, "Created " << num_route_activities
		                                                         << " route filter constraints in "
		                                                         << timer.print() << ".");
	}
}

void RouterSpaceFactory::build_branching(
    RouteCollector::Routes& routes_acyclic, RouteCollector::Routes&)
{
	hate::Timer timer;

	std::set<grenade::common::VertexOnTopology> sources;
	std::set<grenade::common::VertexOnTopology> filter_nodes;

	for (auto const& [route_limits, routes] : routes_acyclic) {
		if (!sources.contains(route_limits.source)) {
			sources.insert(route_limits.source);
		}
		for (auto const& route : routes) {
			for (auto const& intermediate_node : route.descriptors) {
				if (!filter_nodes.contains(intermediate_node)) {
					filter_nodes.insert(intermediate_node);
				}
			}
		}
	}

	Gecode::IntVarArgs source_labels;
	for (auto const& source : sources) {
		auto const& source_vertex =
		    dynamic_cast<Source const&>(m_router_space->m_routing_graph.get(source));
		for (size_t label_index = 0; label_index < source_vertex.size(); label_index++) {
			source_labels << source_vertex.get_label(label_index);
		}
	}

	Gecode::IntVarArgs filter_params;
	for (auto const& filter : filter_nodes) {
		if (auto const* filter_vertex =
		        dynamic_cast<Filter const*>(&m_router_space->m_routing_graph.get(filter));
		    filter_vertex) {
			filter_params << filter_vertex->get_parameters();
		}
	}


	Gecode::branch(
	    *m_router_space, filter_params,
	    Gecode::tiebreak(Gecode::INT_VAR_DEGREE_MAX(), Gecode::INT_VAR_SIZE_MIN()),
	    Gecode::INT_VAL_MAX());
	Gecode::branch(
	    *m_router_space, source_labels,
	    Gecode::tiebreak(Gecode::INT_VAR_DEGREE_MAX(), Gecode::INT_VAR_SIZE_MIN()),
	    Gecode::INT_VAL_MAX());

	assert(m_router_space->m_shared_storage);
	LOG4CXX_DEBUG(
	    m_router_space->m_shared_storage->logger, "Created branching in " << timer.print() << ".");
}

void RouterSpaceFactory::build_tracing()
{
	if (!m_router_space->m_shared_storage->int_tracer.is_enabled()) {
		return;
	}

	hate::Timer timer;
	m_router_space->m_routing_graph.trace(*m_router_space);
	LOG4CXX_DEBUG(
	    m_router_space->m_shared_storage->logger,
	    "Added tracing for parameters in " << timer.print() << ".");
}

} // namespace grenade::vx::network::routing::csp