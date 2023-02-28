#include "grenade/vx/genpybind.h"

GENPYBIND_TAG_GRENADE_VX_LOGICAL_NETWORK
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pyhalco_hicann_dls_vx_v3");
	parent->py::module::import("pyhaldls_vx_v3");
	parent->py::module::import("pylola_vx_v3");
	parent->py::module::import("_pygrenade_vx_execution");
	parent->py::module::import("_pygrenade_vx_signal_flow");
	parent->py::module::import("_pygrenade_vx_network");
})

#include "grenade/vx/logical_network/extract_output.h"
#include "grenade/vx/logical_network/generate_input.h"
#include "grenade/vx/logical_network/network.h"
#include "grenade/vx/logical_network/network_builder.h"
#include "grenade/vx/logical_network/network_graph.h"
#include "grenade/vx/logical_network/network_graph_builder.h"
#include "grenade/vx/logical_network/plasticity_rule.h"
#include "grenade/vx/logical_network/population.h"
#include "grenade/vx/logical_network/projection.h"
