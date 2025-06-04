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

#include "grenade/vx/network/abstract/multicompartment_compartment.h"
#include "grenade/vx/network/abstract/multicompartment_environment.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism/synaptic_conductance.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism/synaptic_current.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism_on_compartment.h"
#include "grenade/vx/network/abstract/multicompartment_neuron.h"
#include "grenade/vx/network/abstract/multicompartment_neuron_circuit.h"
#include "grenade/vx/network/abstract/multicompartment_placement_algorithm.h"
#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_brute_force.h"
#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_dummy.h"
#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_evolutionary.h"
#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_result.h"
#include "grenade/vx/network/abstract/multicompartment_placement_algorithm_ruleset.h"
#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment.h"
#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment/conductance.h"
#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment/current.h"
#include "grenade/vx/network/abstract/multicompartment_top_bottom.h"
#include "grenade/vx/network/abstract/multicompartment_unplaced_neuron_circuit.h"
#include "grenade/vx/network/abstract/parameter_interval.h"
#include "grenade/vx/network/background_source_population.h"
#include "grenade/vx/network/connection_routing_result.h"
#include "grenade/vx/network/external_source_population.h"
#include "grenade/vx/network/extract_output.h"
#include "grenade/vx/network/generate_input.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/network_graph_statistics.h"
#include "grenade/vx/network/pad_recording.h"
#include "grenade/vx/network/plasticity_rule.h"
#include "grenade/vx/network/plasticity_rule_generator.h"
#include "grenade/vx/network/plasticity_rule_on_network.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/population_on_network.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/projection_on_network.h"
#include "grenade/vx/network/requires_routing.h"
#include "grenade/vx/network/routing/portfolio_router.h"
#include "grenade/vx/network/run.h"