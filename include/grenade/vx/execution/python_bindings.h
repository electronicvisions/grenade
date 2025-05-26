#include "grenade/vx/genpybind.h"

GENPYBIND_TAG_GRENADE_VX_EXECUTION
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pyhalco_hicann_dls_vx_v3");
	parent->py::module::import("pystadls_vx_v3");
	parent->py::module::import("pyhxcomm_vx");
	parent->py::module::import("_pygrenade_vx_signal_flow");
})

#include "grenade/vx/execution/execution_instance_hooks.h"
#include "grenade/vx/execution/jit_graph_executor.h"
