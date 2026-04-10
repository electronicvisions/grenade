#pragma once

#include "grenade/vx/network/routing/csp/route_collector.h"
#include "grenade/vx/network/routing/csp/tracer.h"
#include <log4cxx/logger.h>

namespace grenade::vx::network::routing::csp {

/**
 * Storage shared between routing spaces during branching in CSP search.
 */
struct RouterSpaceSharedStorage
{
	RouterSpaceSharedStorage();

	log4cxx::LoggerPtr logger;

	// Shared ownership across complete lifetime of space and offsprings required due to
	// reference-based internal usage.
	IntTracer int_tracer;
};

} // namespace grenade::vx::network::routing::csp