#include "grenade/vx/genpybind.h"

GENPYBIND_TAG_GRENADE_VX_COMMON
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pyhaldls_vx_v3");
})

#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/common/time.h"
