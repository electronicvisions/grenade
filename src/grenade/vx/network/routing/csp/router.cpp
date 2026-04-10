#include "grenade/vx/network/routing/csp/router.h"
#include "grenade/vx/network/routing/csp/router_space.h"
#include "grenade/vx/network/routing/csp/tracer.h"
#include "hate/indent.h"
#include "hate/timer.h"
#include <log4cxx/logger.h>

namespace grenade::vx::network::routing::csp {

Router::Router(RouterOptions const& options) :
    m_options(options), m_logger(log4cxx::Logger::getLogger("grenade.network.routing.csp.Router"))
{
}

Gecode::Search::Options Router::get_search_options() const
{
	Gecode::Search::Options search_options;
	search_options.c_d = m_options.search.commit_distance;
	search_options.a_d = m_options.search.adaptive_distance;
	auto search_tracer = std::make_unique<SearchTracer>();
	if (search_tracer->is_enabled()) {
		search_options.tracer = search_tracer.release();
	}

	return search_options;
}

std::unique_ptr<RouterSpace> Router::operator()(std::unique_ptr<RouterSpace> router_space)
{
	hate::Timer timer_search_next;
	Gecode::DFS<csp::RouterSpace> search_algorithm(router_space.get(), get_search_options());

	std::unique_ptr<csp::RouterSpace> result{search_algorithm.next()};
	if (!result) {
		throw std::runtime_error("CSPRouter didn't find solution.");
	}
	LOG4CXX_DEBUG(m_logger, "Found solution in " << timer_search_next.print() << ".");

	auto const search_statistics = search_algorithm.statistics();
	LOG4CXX_DEBUG(m_logger, "Search depth: " << search_statistics.depth << ".");

	return result;
}

} // namespace grenade::vx::network::routing::csp