#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/routing/router.h"
#include "grenade/vx/network/routing_result.h"
#include "hate/visibility.h"
#include <memory>

#if defined(__GENPYBIND__) or defined(__GENPYBIND_GENERATED__)
#include "grenade/common/linked_topology.h"
#else
namespace grenade::common {
struct LinkedTopology;
} // namespace grenade::common
#endif

namespace grenade::vx::network {
namespace routing GENPYBIND_TAG_GRENADE_VX_NETWORK_ROUTING {

/**
 * Router using a portfolio of routing algorithms.
 * Can be used to find easy or special-case solutions fast with optimized algorithms and using more
 * general algorithms afterwards.
 */
struct SYMBOL_VISIBLE GENPYBIND(
    visible,
    holder_type("std::shared_ptr<grenade::vx::network::routing::PortfolioRouter>")) PortfolioRouter
    : public Router
{
	PortfolioRouter();
	PortfolioRouter(PortfolioRouter const&) = delete;
	PortfolioRouter& operator=(PortfolioRouter const&) = delete;

	PortfolioRouter(std::vector<std::unique_ptr<Router>>&& routers) GENPYBIND(hidden);

	std::vector<std::unique_ptr<Router>> routers GENPYBIND(hidden);

	virtual ~PortfolioRouter();

	virtual RoutingResult operator()(grenade::common::LinkedTopology const& topology) override;
};

} // namespace routing
} // namespace grenade::vx::network
