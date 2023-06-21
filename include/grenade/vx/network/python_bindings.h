#include "grenade/vx/genpybind.h"

GENPYBIND_TAG_GRENADE_VX_NETWORK
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pyhalco_hicann_dls_vx_v3");
	parent->py::module::import("pyhaldls_vx_v3");
	parent->py::module::import("pylola_vx_v3");
	parent->py::module::import("_pygrenade_vx_execution");
	parent->py::module::import("_pygrenade_vx_signal_flow");
})

#include "grenade/vx/network/build_routing.h"
#include "grenade/vx/network/connection_routing_result.h"
#include "grenade/vx/network/extract_output.h"
#include "grenade/vx/network/generate_input.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/network_graph_statistics.h"
#include "grenade/vx/network/plasticity_rule.h"
#include "grenade/vx/network/plasticity_rule_generator.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/requires_routing.h"
#include "grenade/vx/network/run.h"
