#pragma once

#include "grenade/vx/genpybind.h"

GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pyhalco_hicann_dls_vx_v3");
	parent->py::module::import("pyhaldls_vx_v3");
	parent->py::module::import("pylola_vx_v3");
	parent->py::module::import("_pygrenade_vx_execution");
	parent->py::module::import("_pygrenade_vx_signal_flow");
	parent->py::module::import("_pygrenade_vx_network");
})
