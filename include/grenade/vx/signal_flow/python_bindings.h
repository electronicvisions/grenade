#include "grenade/vx/genpybind.h"

GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pyhalco_hicann_dls_vx_v3");
	parent->py::module::import("pyhaldls_vx_v3");
	parent->py::module::import("pylola_vx_v3");
	parent->py::module::import("_pygrenade_vx_common");
})

#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/execution_instance_playback_hooks.h"
#include "grenade/vx/signal_flow/execution_time_info.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "grenade/vx/signal_flow/pack_spikes.h"
#include "grenade/vx/signal_flow/types.h"
