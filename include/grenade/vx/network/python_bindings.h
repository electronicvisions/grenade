#include "grenade/vx/genpybind.h"

GENPYBIND_TAG_GRENADE_VX_NETWORK
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pyhalco_hicann_dls_vx_v3");
	parent->py::module::import("pyhaldls_vx_v3");
	parent->py::module::import("pylola_vx_v3");
	parent->py::module::import("pygrenade_common");
	parent->py::module::import("_pygrenade_vx_execution");
	parent->py::module::import("_pygrenade_vx_signal_flow");
})

#include "grenade/vx/network/connection_routing_result.h"
#include "grenade/vx/network/mapped_topology_statistics.h"
#include "grenade/vx/network/receptor.h"
#include "grenade/vx/signal_flow/types.h"
