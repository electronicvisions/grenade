#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/routing_result.h"
#include "hate/visibility.h"
#include <memory>

#if defined(__GENPYBIND__) or defined(__GENPYBIND_GENERATED__)
#include "grenade/vx/network/network.h"
#else
namespace grenade::vx::network {
struct Network;
} // namespace grenade::vx::network
#endif

namespace grenade::vx::network::routing GENPYBIND_TAG_GRENADE_VX_NETWORK_ROUTING {

struct SYMBOL_VISIBLE Router
{
	virtual ~Router();

	/**
	 * Route given network.
	 * Successive invokations with the same network are allowed to yield different results.
	 * @param network Network to route for
	 * @return Routing result
	 */
	virtual RoutingResult operator()(std::shared_ptr<Network> const& network) = 0;
};

GENPYBIND_MANUAL({
	using namespace ::grenade::vx::network;

	struct PyRouter : public routing::Router
	{
		virtual RoutingResult operator()(std::shared_ptr<Network> const& network) override
		{
			PYBIND11_OVERLOAD_PURE(RoutingResult, routing::Router, operator(), network);
		}
	};

	parent->py::template class_<routing::Router, PyRouter>(parent, "Router")
	    .def("__call__", &routing::Router::operator());
})

} // namespace grenade::vx::network::routing
