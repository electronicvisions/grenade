#include "grenade/vx/genpybind.h"

GENPYBIND_TAG_GRENADE_VX_COMMON
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pygrenade_common");
	parent->py::module::import("pyhaldls_vx_v3");
})

#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/common/time.h"
