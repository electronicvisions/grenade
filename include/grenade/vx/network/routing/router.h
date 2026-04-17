#pragma once
#include "grenade/vx/genpybind.h"
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

struct SYMBOL_VISIBLE Router
{
	virtual ~Router();

	/**
	 * Route given network.
	 * Successive invokations with the same topology are allowed to yield different results.
	 * @param topology Topology to route for
	 * @return Routing result
	 */
	virtual RoutingResult operator()(grenade::common::LinkedTopology const& topology) = 0;
};

GENPYBIND_MANUAL({
	using namespace ::grenade::vx::network;

	struct PyRouter : public routing::Router
	{
		virtual RoutingResult operator()(grenade::common::LinkedTopology const& topology) override
		{
			PYBIND11_OVERLOAD_PURE(RoutingResult, routing::Router, operator(), topology);
		}
	};

	pybind11::class_<routing::Router, PyRouter, std::shared_ptr<routing::Router>>(parent, "Router")
	    .def("__call__", &routing::Router::operator());
})

} // namespace routing
} // namespace grenade::vx::network
