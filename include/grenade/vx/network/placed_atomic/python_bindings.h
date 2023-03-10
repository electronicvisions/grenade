#include "grenade/vx/genpybind.h"

GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_ATOMIC
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pyhalco_hicann_dls_vx_v3");
	parent->py::module::import("pyhaldls_vx_v3");
	parent->py::module::import("pylola_vx_v3");
	parent->py::module::import("_pygrenade_vx_execution");
	parent->py::module::import("_pygrenade_vx_signal_flow");
})

#include "grenade/vx/network/placed_atomic/network.h"
#include "grenade/vx/network/placed_atomic/network_builder.h"
#include "grenade/vx/network/placed_atomic/network_graph.h"
#include "grenade/vx/network/placed_atomic/population.h"
#include "grenade/vx/network/placed_atomic/projection.h"
#include "grenade/vx/network/placed_atomic/run.h"
