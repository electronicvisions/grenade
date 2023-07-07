#include "grenade/vx/genpybind.h"

GENPYBIND_TAG_GRENADE_VX_NETWORK_ROUTING
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("_pygrenade_vx_network");
})

#include "grenade/vx/network/routing/greedy_router.h"
#include "grenade/vx/network/routing/portfolio_router.h"
#include "grenade/vx/network/routing/router.h"
