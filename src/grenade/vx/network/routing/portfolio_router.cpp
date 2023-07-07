#include "grenade/vx/network/routing/portfolio_router.h"

#include "grenade/vx/network/exception.h"
#include "grenade/vx/network/routing/greedy_router.h"
#include <stdexcept>
#include <log4cxx/logger.h>

namespace grenade::vx::network::routing {

PortfolioRouter::PortfolioRouter() : routers()
{
	routers.emplace_back(std::make_unique<GreedyRouter>());
}

PortfolioRouter::~PortfolioRouter() {}

RoutingResult PortfolioRouter::operator()(std::shared_ptr<Network> const& network)
{
	for (size_t i = 0; i < routers.size(); ++i) {
		auto& router = routers.at(i);
		if (!router) {
			throw std::logic_error("Unexpected access to moved-from object.");
		}
		try {
			return (*router)(network);
		} catch (UnsuccessfulRouting const& exception) {
			auto const logger =
			    log4cxx::Logger::getLogger("grenade.network.routing.PortfolioRouter");
			LOG4CXX_TRACE(
			    logger, "Router " << (i + 1) << "/" << routers.size()
			                      << " unsuccessful:" << std::string(exception.what()));
		}
	}
	throw UnsuccessfulRouting("No router of portfolio was successful.");
}

} // namespace grenade::vx::network::routing
