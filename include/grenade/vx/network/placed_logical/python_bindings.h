#include "grenade/vx/genpybind.h"

GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pyhalco_hicann_dls_vx_v3");
	parent->py::module::import("pyhaldls_vx_v3");
	parent->py::module::import("pylola_vx_v3");
	parent->py::module::import("_pygrenade_vx_execution");
	parent->py::module::import("_pygrenade_vx_signal_flow");
	parent->py::module::import("_pygrenade_vx_network_placed_atomic");
})

#include "grenade/vx/network/placed_logical/build_routing.h"
#include "grenade/vx/network/placed_logical/connection_routing_result.h"
#include "grenade/vx/network/placed_logical/extract_output.h"
#include "grenade/vx/network/placed_logical/generate_input.h"
#include "grenade/vx/network/placed_logical/network.h"
#include "grenade/vx/network/placed_logical/network_builder.h"
#include "grenade/vx/network/placed_logical/network_graph.h"
#include "grenade/vx/network/placed_logical/network_graph_builder.h"
#include "grenade/vx/network/placed_logical/network_graph_statistics.h"
#include "grenade/vx/network/placed_logical/plasticity_rule.h"
#include "grenade/vx/network/placed_logical/plasticity_rule_generator.h"
#include "grenade/vx/network/placed_logical/population.h"
#include "grenade/vx/network/placed_logical/projection.h"
#include "grenade/vx/network/placed_logical/requires_routing.h"
#include "grenade/vx/network/placed_logical/run.h"
