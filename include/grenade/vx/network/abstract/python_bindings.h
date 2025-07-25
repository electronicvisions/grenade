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

#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/multi_index_sequence_dimension_unit/atomic_neuron_on_compartment.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated_signed.h"
#include "grenade/vx/network/abstract/vertex_port_type/spike.h"
#include "grenade/vx/network/abstract/vertex_port_type/synapse_observable.h"
#include "grenade/vx/network/abstract/vertex_port_type/synaptic_input.h"
#include "grenade/vx/network/abstract/vertex_port_type/weight.h"
