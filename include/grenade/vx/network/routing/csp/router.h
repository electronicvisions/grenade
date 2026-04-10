#pragma once

#include "grenade/vx/network/routing/csp/router_options.h"
#include "grenade/vx/network/routing/csp/router_space.h"
#include "grenade/vx/network/routing/csp/source_target_pair.h"
#include "hate/visibility.h"
#include <gecode/search.hh>

namespace log4cxx {
struct Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx


namespace grenade::vx::network::routing::csp {

/**
 * Constraint satisfaction router.
 *
 * Performs a DFS on a routing space to find a unique solution.
 */
struct SYMBOL_VISIBLE Router
{
	Router(RouterOptions const& options = RouterOptions());

	/**
	 * Perform search for unique routing solution.
	 * @param router_space Router space with initial constraints.
	 * @return Fully constrained router space.
	 */
	std::unique_ptr<RouterSpace> operator()(std::unique_ptr<RouterSpace> router_space);

private:
	Gecode::Search::Options get_search_options() const;

	RouterOptions m_options;

	log4cxx::LoggerPtr m_logger;
};

} // namespace grenade::vx::network::routing::csp