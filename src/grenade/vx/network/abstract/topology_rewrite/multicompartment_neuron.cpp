#include "grenade/vx/network/abstract/topology_rewrite/multicompartment_neuron.h"

#include "ccalix/types.h"
#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/multi_index_sequence_dimension_unit/receptor_on_compartment.h"
#include "grenade/common/projection.h"
#include "grenade/common/projection_connector/static.h"
#include "grenade/common/receptor_on_compartment.h"
#include "grenade/vx/network/abstract/mapping/multicompartment_neuron.h"
#include "grenade/vx/network/abstract/multi_index_sequence_dimension_unit/atomic_neuron_on_compartment.h"
#include "grenade/vx/network/abstract/multi_index_sequence_dimension_unit/mechanism_on_compartment.h"
#include "grenade/vx/network/abstract/multicompartment/compartment_connection/conductance.h"
#include "grenade/vx/network/abstract/multicompartment/compartment_connection_on_neuron.h"
#include "grenade/vx/network/abstract/multicompartment/environment.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_constraint.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/analog_readout.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_excitatory.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_inhibitory.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/fire.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/leak.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_conductance.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_current.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_input.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/with_analog_readout.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_environment/synaptic_input.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_on_compartment.h"
#include "grenade/vx/network/abstract/multicompartment/neuron.h"
#include "grenade/vx/network/abstract/multicompartment/placement/coordinate_system.h"
#include "grenade/vx/network/abstract/multicompartment/resource_manager.h"
#include "grenade/vx/network/abstract/population_cell/calibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/vertex_port_type/analog_observable.h"
#include "grenade/vx/network/abstract/vertex_port_type/spike.h"
#include "grenade/vx/network/abstract/vertex_port_type/synaptic_input.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/math.h"
#include "lola/vx/v3/neuron.h"
#include <boost/bimap.hpp>

namespace grenade::vx::network::abstract {

MulticompartmentNeuronRewrite::MulticompartmentNeuronRewrite(
    std::shared_ptr<grenade::common::LinkedTopology> topology,
    std::unique_ptr<PlacementAlgorithm> placement_algorithm) :
    TopologyRewrite(std::move(topology)), m_placement_algorithm(std::move(placement_algorithm))
{
}

void MulticompartmentNeuronRewrite::check_is_single_operation_point(
    Neuron::ParameterSpace const& model_parameter_space) const
{
	auto const check_mechanism_parameter = [](auto const& parameter) {
		if (std::ranges::any_of(parameter, [](auto const& interval) {
			    return interval.get_lower() != interval.get_upper();
		    })) {
			throw std::runtime_error(
			    "Mechanism parameter doesn't contain "
			    "single-operation point calibration target, i.e. the parameter upper and lower "
			    "limit of the parameter space are different.");
		}
	};

	for (auto const& [_, compartment] : model_parameter_space.compartments) {
		for (auto const& [_, mechanism] : compartment.mechanisms) {
			if (auto const mechanism_capacitance =
			        dynamic_cast<MechanismCapacitance::ParameterSpace const*>(&mechanism);
			    mechanism_capacitance) {
				check_mechanism_parameter(mechanism_capacitance->capacitance);
			} else if (auto const mechanism_leak =
			               dynamic_cast<MechanismLeak::ParameterSpace const*>(&mechanism);
			           mechanism_leak) {
				check_mechanism_parameter(mechanism_leak->time_constant);
				check_mechanism_parameter(mechanism_leak->potential);
			} else if (auto const mechanism_synaptic_conductance =
			               dynamic_cast<MechanismSynapticInputConductance::ParameterSpace const*>(
			                   &mechanism);
			           mechanism_synaptic_conductance) {
				check_mechanism_parameter(mechanism_synaptic_conductance->e_reference);
				check_mechanism_parameter(mechanism_synaptic_conductance->e_reversal);
				check_mechanism_parameter(mechanism_synaptic_conductance->time_constant);
				check_mechanism_parameter(mechanism_synaptic_conductance->i_synin_gm);
				check_mechanism_parameter(mechanism_synaptic_conductance->synapse_dac_bias);
			} else if (auto const mechanism_synaptic_current =
			               dynamic_cast<MechanismSynapticInputCurrent::ParameterSpace const*>(
			                   &mechanism);
			           mechanism_synaptic_current) {
				check_mechanism_parameter(mechanism_synaptic_current->time_constant);
				check_mechanism_parameter(mechanism_synaptic_current->i_synin_gm);
				check_mechanism_parameter(mechanism_synaptic_current->synapse_dac_bias);
			} else if (auto const mechanism_fire =
			               dynamic_cast<MechanismFire::ParameterSpace const*>(&mechanism);
			           mechanism_fire) {
				check_mechanism_parameter(mechanism_fire->threshold_potential);
				check_mechanism_parameter(mechanism_fire->reset_potential);
				check_mechanism_parameter(mechanism_fire->refractory_time);
				check_mechanism_parameter(mechanism_fire->holdoff_time);
			}
		}
	}
}

Environment MulticompartmentNeuronRewrite::construct_environment(
    grenade::common::VertexOnTopology vertex_on_topology,
    grenade::common::Population const& model_population) const
{
	std::map<
	    grenade::common::CompartmentOnNeuron,
	    std::map<MechanismOnCompartment, SynapticInputEnvironment>>
	    resources;
	auto const& neuron = dynamic_cast<Neuron const&>(model_population.get_cell());
	for (auto const& compartment_descriptor : neuron.compartments()) {
		auto const& compartment = neuron.get(compartment_descriptor);
		for (auto const& [mechanism_descriptor, mechanism] : compartment.mechanisms) {
			if (auto const synaptic_input = dynamic_cast<MechanismSynapticInput const*>(&mechanism);
			    synaptic_input) {
				resources[compartment_descriptor][mechanism_descriptor] = {};
			}
		}
	}

	for (auto const in_edge_descriptor : get_topology().in_edges(vertex_on_topology)) {
		auto const& in_edge = get_topology().get(in_edge_descriptor);
		if (auto const projection = dynamic_cast<grenade::common::Projection const*>(
		        &get_topology().get(get_topology().source(in_edge_descriptor)));
		    projection) {
			auto const connector_output_sequence =
			    projection->get_connector().get_output_sequence();
			auto const connector_output_sequence_elements =
			    connector_output_sequence->get_elements();
			auto const connector_input_sequence = projection->get_connector().get_input_sequence();
			auto const output_port = projection->get_output_ports().at(in_edge.port_on_source);
			if (auto const static_connector = dynamic_cast<grenade::common::StaticConnector const*>(
			        &projection->get_connector());
			    static_connector) {
				auto const synapse_locations = static_connector->get_synapse_connections(
				    *connector_input_sequence->cartesian_product(*connector_output_sequence));

				std::set<size_t> output_dimensions;
				for (size_t i = 0; i < connector_output_sequence->dimensionality(); ++i) {
					output_dimensions.insert(i + connector_input_sequence->dimensionality());
				}
				auto synapse_output_locations = synapse_locations->projection(output_dimensions);
				auto synapse_output_location_elements = synapse_output_locations->get_elements();

				auto const synapses_on_edge_elements =
				    grenade::common::CuboidMultiIndexSequence({synapse_output_locations->size()})
				        .related_sequence_subset_restriction(
				            *synapse_output_locations,
				            *connector_output_sequence->related_sequence_subset_restriction(
				                output_port.get_channels(), in_edge.get_channels_on_source()))
				        ->get_elements();

				if (auto const parameter_space =
				        dynamic_cast<UncalibratedSynapse::ParameterSpace const*>(
				            &projection->get_synapse_parameter_space());
				    parameter_space) {
					if (parameter_space->size() != synapse_output_locations->size()) {
						throw std::runtime_error(
						    "Only connectors with parameterization per synapse supported:" +
						    std::to_string(parameter_space->size()) + " vs. " +
						    std::to_string(synapse_output_locations->size()));
					}
					// find mechanism on compartment dimension
					auto const model_population_input_port_dimension_units =
					    model_population.get_input_ports()
					        .at(in_edge.port_on_target)
					        .get_channels()
					        .get_dimension_units();
					size_t const mechanism_dimension = std::distance(
					    model_population_input_port_dimension_units.begin(),
					    std::find(
					        model_population_input_port_dimension_units.begin(),
					        model_population_input_port_dimension_units.end(),
					        MechanismOnCompartmentDimensionUnit()));
					if (mechanism_dimension == model_population_input_port_dimension_units.size()) {
						throw std::runtime_error(
						    "Model neuron input port doesn't feature a mechanism on "
						    "compartment dimension.");
					}
					// find compartment on neuron dimension
					size_t const compartment_dimension = std::distance(
					    model_population_input_port_dimension_units.begin(),
					    std::find(
					        model_population_input_port_dimension_units.begin(),
					        model_population_input_port_dimension_units.end(),
					        grenade::common::CompartmentOnNeuronDimensionUnit()));
					if (compartment_dimension ==
					    model_population_input_port_dimension_units.size()) {
						throw std::runtime_error(
						    "Model neuron output port doesn't feature a compartment on "
						    "cell dimension.");
					}
					auto const channels_on_source_elements =
					    in_edge.get_channels_on_source().get_elements();
					auto const& channels_on_target_elements =
					    in_edge.get_channels_on_target()
					        .projection({compartment_dimension, mechanism_dimension})
					        ->get_elements();
					auto const output_port_elements = output_port.get_channels().get_elements();
					for (auto const& synapse_on_edge : synapses_on_edge_elements) {
						size_t output_on_connector = std::distance(
						    connector_output_sequence_elements.begin(),
						    std::find(
						        connector_output_sequence_elements.begin(),
						        connector_output_sequence_elements.end(),
						        synapse_output_location_elements.at(synapse_on_edge.value.at(0))));
						size_t channel_index_on_edge = std::distance(
						    channels_on_source_elements.begin(),
						    std::find(
						        channels_on_source_elements.begin(),
						        channels_on_source_elements.end(),
						        output_port_elements.at(output_on_connector)));
						auto const& channel_on_target =
						    channels_on_target_elements.at(channel_index_on_edge);

						grenade::common::CompartmentOnNeuron compartment_on_neuron(
						    channel_on_target.value.at(0));
						MechanismOnCompartment mechanism_on_compartment(
						    channel_on_target.value.at(1));
						auto const& max_weight =
						    parameter_space->max_weights.at(synapse_on_edge.value.at(0));

						resources.at(compartment_on_neuron)
						    .at(mechanism_on_compartment)
						    .number_of_inputs.number_total += hate::math::round_up_integer_division(
						    max_weight, lola::vx::v3::SynapseMatrix::Weight::max);
					}
				} else {
					throw std::runtime_error("Handling synapse type not implemented.");
				}
			} else {
				throw std::runtime_error("Handling connector not implemented.");
			}
		}
	}

	Environment environment;
	for (auto const& [compartment_on_neuron, local_env] : resources) {
		for (auto const& [mechanism_on_compartment, local_mechanism_env] : local_env) {
			environment.add(
			    grenade::common::CompartmentOnNeuron(compartment_on_neuron),
			    mechanism_on_compartment, local_mechanism_env);
		}
	}
	return environment;
}

std::tuple<
    CalibratedNeuron,
    std::map<
        grenade::common::CompartmentOnNeuron,
        std::map<MechanismOnCompartment, std::set<size_t>>>,
    std::map<grenade::common::CompartmentOnNeuron, std::map<size_t, MechanismOnCompartment>>>
MulticompartmentNeuronRewrite::construct_calibrated_neuron(
    halco::hicann_dls::vx::v3::LogicalNeuronCompartments const& logical_neuron_compartments,
    Neuron const& model_neuron,
    Neuron::ParameterSpace const& model_parameter_space,
    Environment const& environment) const
{
	std::map<
	    grenade::common::CompartmentOnNeuron, std::map<MechanismOnCompartment, std::set<size_t>>>
	    mechanism_on_atomic_neuron_placement;

	std::map<
	    grenade::common::CompartmentOnNeuron,
	    boost::bimap<
	        halco::hicann_dls::vx::v3::AtomicNeuronOnLogicalNeuron, MechanismOnCompartment>>
	    mechanism_readout_placement;

	CalibratedNeuron::Compartments compartments;
	for (auto const& [compartment_on_neuron, atomic_neurons] :
	     logical_neuron_compartments.get_compartments()) {
		auto const& model_compartment =
		    model_neuron.get(grenade::common::CompartmentOnNeuron(compartment_on_neuron));
		auto compartment_environment =
		    environment.get(grenade::common::CompartmentOnNeuron(compartment_on_neuron));
		std::optional<CalibratedNeuron::Compartment::SpikeMaster> spike_master;
		CalibratedNeuron::Compartment::Receptors receptors;

		auto& mechanism_readout_placement_on_compartment =
		    mechanism_readout_placement[grenade::common::CompartmentOnNeuron(
		        compartment_on_neuron)];

		// synaptic input usage to track placement of synaptic input mechanisms
		std::map<
		    halco::hicann_dls::vx::v3::AtomicNeuronOnLogicalNeuron,
		    std::map<CalibratedNeuron::Compartment::ReceptorType, MechanismOnCompartment>>
		    synaptic_input_usage;
		for (auto const& atomic_neuron : atomic_neurons) {
			synaptic_input_usage[atomic_neuron] = {};
		}

		for (auto const& [mechanism_on_compartment, mechanism] : model_compartment.mechanisms) {
			if (auto const event_output_mechanism = dynamic_cast<MechanismFire const*>(&mechanism);
			    event_output_mechanism) {
				// choose the first neuron circuit of the compartment to be the spike
				// master
				assert(!spike_master);
				spike_master = CalibratedNeuron::Compartment::SpikeMaster(0);
				// store mechanism for use in topology edge rewrite
				mechanism_on_atomic_neuron_placement[grenade::common::CompartmentOnNeuron(
				    compartment_on_neuron)][mechanism_on_compartment]
				    .insert(0);
			}
		}
		// first place readout mechanisms which have top/bottom constraints of readout
		for (auto const& [mechanism_on_compartment, mechanism] : model_compartment.mechanisms) {
			if (auto const analog_readout_mechanism =
			        dynamic_cast<MechanismWithAnalogReadout const*>(&mechanism);
			    analog_readout_mechanism && analog_readout_mechanism->enable_analog_readout) {
				MechanismEnvironment const* mechanism_environment = nullptr;
				if (compartment_environment.contains(mechanism_on_compartment)) {
					mechanism_environment =
					    &(*compartment_environment.at(mechanism_on_compartment));
				}
				auto const hardware_constraints = analog_readout_mechanism->get_hardware(
				    model_parameter_space.compartments
				        .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
				        .mechanisms.get(mechanism_on_compartment),
				    mechanism_environment);
				HardwareConstraint hardware_constraint;
				for (auto const& hc : hardware_constraints) {
					if (dynamic_cast<HardwareResourceAnalogReadout*>(&(*hc->resource))) {
						hardware_constraint = *hc;
						break;
					}
				}
				assert(hardware_constraint.numbers.number_total == 1);
				if (hardware_constraint.numbers.number_top != 0 ||
				    hardware_constraint.numbers.number_bottom != 0) {
					bool mechanism_placed = false;
					for (auto const& atomic_neuron : atomic_neurons) {
						// skipping circuits which are already in use for readout
						if (mechanism_readout_placement_on_compartment.left.find(atomic_neuron) !=
						    mechanism_readout_placement_on_compartment.left.end()) {
							continue;
						}
						if ((hardware_constraint.numbers.number_top != 0 &&
						     atomic_neuron.toNeuronRowOnLogicalNeuron() ==
						         halco::hicann_dls::vx::v3::NeuronRowOnDLS::top) ||
						    (hardware_constraint.numbers.number_bottom != 0 &&
						     atomic_neuron.toNeuronRowOnLogicalNeuron() ==
						         halco::hicann_dls::vx::v3::NeuronRowOnDLS::bottom)) {
							mechanism_on_atomic_neuron_placement
							    [grenade::common::CompartmentOnNeuron(compartment_on_neuron)]
							    [mechanism_on_compartment]
							        .insert(std::distance(
							            atomic_neurons.begin(),
							            std::find(
							                atomic_neurons.begin(), atomic_neurons.end(),
							                atomic_neuron)));
							mechanism_readout_placement_on_compartment.left.insert(
							    {atomic_neuron, mechanism_on_compartment});
							if (auto const synaptic_input_mechanism =
							        dynamic_cast<MechanismSynapticInput const*>(&mechanism);
							    synaptic_input_mechanism) {
								synaptic_input_usage[atomic_neuron].emplace(
								    synaptic_input_mechanism->receptor_type,
								    mechanism_on_compartment);
							}
							mechanism_placed = true;
							break;
						}
					}
					assert(mechanism_placed);
				}
			}
		}
		// then place readout for leftover mechanisms without top/bottom constraints for readout
		// part
		for (auto const& [mechanism_on_compartment, mechanism] : model_compartment.mechanisms) {
			if (auto const analog_readout_mechanism =
			        dynamic_cast<MechanismWithAnalogReadout const*>(&mechanism);
			    analog_readout_mechanism && analog_readout_mechanism->enable_analog_readout &&
			    mechanism_readout_placement_on_compartment.right.find(mechanism_on_compartment) ==
			        mechanism_readout_placement_on_compartment.right.end()) {
				MechanismEnvironment const* mechanism_environment = nullptr;
				if (compartment_environment.contains(mechanism_on_compartment)) {
					mechanism_environment =
					    &(*compartment_environment.at(mechanism_on_compartment));
				}
				auto const hardware_constraints = analog_readout_mechanism->get_hardware(
				    model_parameter_space.compartments
				        .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
				        .mechanisms.get(mechanism_on_compartment),
				    mechanism_environment);
				HardwareConstraint hardware_constraint;
				for (auto const& hc : hardware_constraints) {
					if (dynamic_cast<HardwareResourceAnalogReadout*>(&(*hc->resource))) {
						hardware_constraint = *hc;
						break;
					}
				}
				assert(hardware_constraint.numbers.number_total == 1);
				assert(hardware_constraint.numbers.number_top == 0);
				assert(hardware_constraint.numbers.number_bottom == 0);
				bool mechanism_placed = false;
				for (auto const& atomic_neuron : atomic_neurons) {
					// skipping circuits which are already in use for readout
					if (mechanism_readout_placement_on_compartment.left.find(atomic_neuron) !=
					    mechanism_readout_placement_on_compartment.left.end()) {
						continue;
					}
					mechanism_on_atomic_neuron_placement[grenade::common::CompartmentOnNeuron(
					    compartment_on_neuron)][mechanism_on_compartment]
					    .insert(std::distance(
					        atomic_neurons.begin(),
					        std::find(
					            atomic_neurons.begin(), atomic_neurons.end(), atomic_neuron)));
					mechanism_readout_placement_on_compartment.left.insert(
					    {atomic_neuron, mechanism_on_compartment});
					if (auto const synaptic_input_mechanism =
					        dynamic_cast<MechanismSynapticInput const*>(&mechanism);
					    synaptic_input_mechanism) {
						synaptic_input_usage[atomic_neuron].emplace(
						    synaptic_input_mechanism->receptor_type, mechanism_on_compartment);
					}
					mechanism_placed = true;
					break;
				}
				assert(mechanism_placed);
			}
		}

		// first place synaptic inputs with top/bottom constraints
		for (auto const& [mechanism_on_compartment, mechanism] : model_compartment.mechanisms) {
			if (auto const synaptic_input_mechanism =
			        dynamic_cast<MechanismSynapticInput const*>(&mechanism);
			    synaptic_input_mechanism) {
				auto const& mechanism_environment =
				    compartment_environment[mechanism_on_compartment];
				assert(mechanism_environment);
				auto const& synaptic_input_environment =
				    dynamic_cast<SynapticInputEnvironment const&>(*mechanism_environment);
				auto const hardware_constraints = synaptic_input_mechanism->get_hardware(
				    model_parameter_space.compartments
				        .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
				        .mechanisms.get(mechanism_on_compartment),
				    &synaptic_input_environment);
				HardwareConstraint hardware_constraint;
				for (auto const& hc : hardware_constraints) {
					if (dynamic_cast<HardwareResourceSynapticInputExitatory*>(&(*hc->resource))) {
						hardware_constraint = *hc;
						break;
					}
					if (dynamic_cast<HardwareResourceSynapticInputInhibitory*>(&(*hc->resource))) {
						hardware_constraint = *hc;
						break;
					}
				}
				// start iterating from the number of already placed circuits for the read out
				// mechanism part
				for (size_t i = mechanism_readout_placement_on_compartment.right.find(
				                    mechanism_on_compartment) !=
				                        mechanism_readout_placement_on_compartment.right.end()
				                    ? mechanism_readout_placement_on_compartment.right
				                              .at(mechanism_on_compartment)
				                              .toNeuronRowOnLogicalNeuron() ==
				                          halco::hicann_dls::vx::v3::NeuronRowOnDLS::top
				                    : 0;
				     i < hardware_constraint.numbers.number_top; ++i) {
					bool mechanism_placed = false;
					for (auto& [atomic_neuron, local_receptors] : synaptic_input_usage) {
						if (atomic_neuron.toNeuronRowOnLogicalNeuron() ==
						        halco::hicann_dls::vx::v3::NeuronRowOnDLS::top &&
						    !local_receptors.contains(synaptic_input_mechanism->receptor_type)) {
							local_receptors.emplace(
							    synaptic_input_mechanism->receptor_type, mechanism_on_compartment);
							mechanism_on_atomic_neuron_placement
							    [grenade::common::CompartmentOnNeuron(compartment_on_neuron)]
							    [mechanism_on_compartment]
							        .insert(std::distance(
							            atomic_neurons.begin(),
							            std::find(
							                atomic_neurons.begin(), atomic_neurons.end(),
							                atomic_neuron)));
							mechanism_placed = true;
							break;
						}
					}
					assert(mechanism_placed);
				}
				// start iterating from the number of already placed circuits for the read out
				// mechanism part
				for (size_t i = mechanism_readout_placement_on_compartment.right.find(
				                    mechanism_on_compartment) !=
				                        mechanism_readout_placement_on_compartment.right.end()
				                    ? mechanism_readout_placement_on_compartment.right
				                              .at(mechanism_on_compartment)
				                              .toNeuronRowOnLogicalNeuron() ==
				                          halco::hicann_dls::vx::v3::NeuronRowOnDLS::bottom
				                    : 0;
				     i < hardware_constraint.numbers.number_bottom; ++i) {
					bool mechanism_placed = false;
					for (auto& [atomic_neuron, local_receptors] : synaptic_input_usage) {
						if (atomic_neuron.toNeuronRowOnLogicalNeuron() ==
						        halco::hicann_dls::vx::v3::NeuronRowOnDLS::bottom &&
						    !local_receptors.contains(synaptic_input_mechanism->receptor_type)) {
							local_receptors.emplace(
							    synaptic_input_mechanism->receptor_type, mechanism_on_compartment);
							mechanism_on_atomic_neuron_placement
							    [grenade::common::CompartmentOnNeuron(compartment_on_neuron)]
							    [mechanism_on_compartment]
							        .insert(std::distance(
							            atomic_neurons.begin(),
							            std::find(
							                atomic_neurons.begin(), atomic_neurons.end(),
							                atomic_neuron)));
							mechanism_placed = true;
							break;
						}
					}
					assert(mechanism_placed);
				}
			}
		}
		// then place leftover synaptic inputs without top/bottom constraints
		for (auto const& [mechanism_on_compartment, mechanism] : model_compartment.mechanisms) {
			if (auto const synaptic_input_mechanism =
			        dynamic_cast<MechanismSynapticInput const*>(&mechanism);
			    synaptic_input_mechanism) {
				auto const& mechanism_environment =
				    compartment_environment[mechanism_on_compartment];
				assert(mechanism_environment);
				auto const& synaptic_input_environment =
				    dynamic_cast<SynapticInputEnvironment const&>(*mechanism_environment);
				auto const hardware_constraints = synaptic_input_mechanism->get_hardware(
				    model_parameter_space.compartments
				        .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
				        .mechanisms.get(mechanism_on_compartment),
				    &synaptic_input_environment);
				HardwareConstraint hardware_constraint;
				for (auto const& hc : hardware_constraints) {
					if (dynamic_cast<HardwareResourceSynapticInputExitatory*>(&(*hc->resource))) {
						hardware_constraint = *hc;
						break;
					}
					if (dynamic_cast<HardwareResourceSynapticInputInhibitory*>(&(*hc->resource))) {
						hardware_constraint = *hc;
						break;
					}
				}
				// start iterating from the not yet accounted for number of already placed circuits
				// for the read out mechanism part
				for (size_t i =
				         (hardware_constraint.numbers.number_top == 0 &&
				          hardware_constraint.numbers.number_bottom == 0 &&
				          (mechanism_readout_placement_on_compartment.right.find(
				               mechanism_on_compartment) !=
				           mechanism_readout_placement_on_compartment.right.end()));
				     i < hardware_constraint.numbers.number_total -
				             hardware_constraint.numbers.number_top -
				             hardware_constraint.numbers.number_bottom;
				     ++i) {
					bool mechanism_placed = false;
					for (auto const& atomic_neuron : atomic_neurons) {
						if (!synaptic_input_usage[atomic_neuron].contains(
						        synaptic_input_mechanism->receptor_type)) {
							synaptic_input_usage[atomic_neuron].emplace(
							    synaptic_input_mechanism->receptor_type, mechanism_on_compartment);
							mechanism_on_atomic_neuron_placement
							    [grenade::common::CompartmentOnNeuron(compartment_on_neuron)]
							    [mechanism_on_compartment]
							        .insert(std::distance(
							            atomic_neurons.begin(),
							            std::find(
							                atomic_neurons.begin(), atomic_neurons.end(),
							                atomic_neuron)));
							mechanism_placed = true;
							break;
						}
					}
					assert(mechanism_placed);
				}
			}
		}

		// map MechanismOnCompartment to ReceptorOnCompartment
		receptors.resize(atomic_neurons.size());
		for (size_t atomic_neuron_on_compartment = 0;
		     atomic_neuron_on_compartment < atomic_neurons.size(); ++atomic_neuron_on_compartment) {
			for (auto const& [receptor_type, mechanism_on_compartment] :
			     synaptic_input_usage.at(atomic_neurons.at(atomic_neuron_on_compartment))) {
				receptors.at(atomic_neuron_on_compartment)
				    .emplace(
				        grenade::common::ReceptorOnCompartment(mechanism_on_compartment.value()),
				        receptor_type);
			}
		}

		compartments.emplace(
		    grenade::common::CompartmentOnNeuron(compartment_on_neuron),
		    CalibratedNeuron::Compartment(std::move(spike_master), std::move(receptors)));
	}

	std::map<grenade::common::CompartmentOnNeuron, std::map<size_t, MechanismOnCompartment>>
	    mechanism_readout_placement_indices;

	for (auto const& [compartment_on_neuron, atomic_neurons] :
	     logical_neuron_compartments.get_compartments()) {
		if (!mechanism_readout_placement.contains(
		        grenade::common::CompartmentOnNeuron(compartment_on_neuron))) {
			continue;
		}
		for (auto const& [atomic_neuron, mechanism_on_compartment] : mechanism_readout_placement.at(
		         grenade::common::CompartmentOnNeuron(compartment_on_neuron))) {
			mechanism_readout_placement_indices[grenade::common::CompartmentOnNeuron(
			                                        compartment_on_neuron)]
			    .emplace(
			        std::distance(
			            atomic_neurons.begin(),
			            std::find(atomic_neurons.begin(), atomic_neurons.end(), atomic_neuron)),
			        mechanism_on_compartment);
		}
	}

	return {
	    CalibratedNeuron(std::move(compartments), logical_neuron_compartments),
	    std::move(mechanism_on_atomic_neuron_placement),
	    std::move(mechanism_readout_placement_indices)};
}

MulticompartmentNeuronMapping MulticompartmentNeuronRewrite::construct_mapping(
    Neuron const& model_neuron,
    size_t population_size,
    halco::hicann_dls::vx::v3::LogicalNeuronCompartments const& logical_neuron_compartments,
    std::map<grenade::common::CompartmentOnNeuron, std::map<size_t, MechanismOnCompartment>> const&
        mechanism_readout_placement) const
{
	MulticompartmentNeuronMapping::ReadoutSources readout_sources(population_size);
	for (size_t neuron_on_population = 0; neuron_on_population < population_size;
	     ++neuron_on_population) {
		for (auto const& [compartment_on_neuron, atomic_neurons] :
		     logical_neuron_compartments.get_compartments()) {
			auto& compartment_readout_sources = readout_sources.at(
			    neuron_on_population)[grenade::common::CompartmentOnNeuron(compartment_on_neuron)];
			compartment_readout_sources.resize(atomic_neurons.size());
			if (!mechanism_readout_placement.contains(
			        grenade::common::CompartmentOnNeuron(compartment_on_neuron))) {
				continue;
			}
			for (auto const& [an, mechanism_on_compartment] : mechanism_readout_placement.at(
			         grenade::common::CompartmentOnNeuron(compartment_on_neuron))) {
				compartment_readout_sources.at(an) =
				    dynamic_cast<MechanismWithAnalogReadout const&>(
				        model_neuron
				            .get(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
				            .mechanisms.get(mechanism_on_compartment))
				        .get_analog_readout_source();
			}
		}
	}

	return MulticompartmentNeuronMapping(std::move(readout_sources));
}

CalibratedNeuron::ParameterSpace
MulticompartmentNeuronRewrite::construct_calibrated_neuron_parameter_space(
    CalibratedNeuron const& calibrated_neuron,
    halco::hicann_dls::vx::v3::LogicalNeuronCompartments const& logical_neuron_compartments,
    AlgorithmResult const& local_placement,
    Neuron const& model_neuron,
    Neuron::ParameterSpace const& model_parameter_space,
    std::map<
        grenade::common::CompartmentOnNeuron,
        std::map<MechanismOnCompartment, std::set<size_t>>>& mechanism_on_atomic_neuron_placement)
    const
{
	CalibratedNeuron::ParameterSpace::CalibrationTargets calibration_targets(
	    model_parameter_space.size());

	CalibratedNeuron::ParameterSpace::MembraneCapacitance membrane_capacitances(
	    model_parameter_space.size());

	for (auto const& [compartment_on_neuron, atomic_neurons] :
	     logical_neuron_compartments.get_compartments()) {
		auto const& model_compartment =
		    model_neuron.get(grenade::common::CompartmentOnNeuron(compartment_on_neuron));

		for (size_t neuron_on_population = 0; neuron_on_population < calibration_targets.size();
		     ++neuron_on_population) {
			calibration_targets
			    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
			        compartment_on_neuron)]
			    .resize(atomic_neurons.size());
			// disable mechanism to enable selected below
			for (auto& an : calibration_targets.at(neuron_on_population)
			                    .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))) {
				an.v_threshold = std::nullopt;
				an.tau_membrane = std::nullopt;
				an.synaptic_input_excitatory =
				    CalibratedNeuron::ParameterSpace::CalibrationTarget::DisabledSynapticInput();
				an.synaptic_input_inhibitory =
				    CalibratedNeuron::ParameterSpace::CalibrationTarget::DisabledSynapticInput();
			}
		}

		for (auto const& [mechanism_on_compartment, mechanism] : model_compartment.mechanisms) {
			if (auto const capacitance_mechanism =
			        dynamic_cast<MechanismCapacitance const*>(&mechanism);
			    capacitance_mechanism) {
				auto const& mechanism_parameter_space =
				    dynamic_cast<MechanismCapacitance::ParameterSpace const&>(
				        model_parameter_space.compartments
				            .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
				            .mechanisms.get(mechanism_on_compartment));
				for (size_t neuron_on_population = 0;
				     neuron_on_population < calibration_targets.size(); ++neuron_on_population) {
					membrane_capacitances.at(
					    neuron_on_population)[grenade::common::CompartmentOnNeuron(
					    compartment_on_neuron)] =
					    mechanism_parameter_space.capacitance.at(neuron_on_population).get_lower();
					// place capacitance on first neuron circuit
					mechanism_on_atomic_neuron_placement[grenade::common::CompartmentOnNeuron(
					    compartment_on_neuron)][mechanism_on_compartment]
					    .insert(0);
					// set calibration targets for the capacitance -> place whole capacitance
					// on first circuit
					for (size_t an = 0; an < atomic_neurons.size(); ++an) {
						// set leak potential on all circuits
						calibration_targets
						    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
						        compartment_on_neuron)]
						    .at(an)
						    .membrane_capacitance_during_calibration =
						    ccalix::CapacitanceInFarad(2.2e-12);
					}
					calibration_targets
					    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
					        compartment_on_neuron)]
					    .at(0)
					    .membrane_capacitance_during_calibration =
					    mechanism_parameter_space.capacitance.at(neuron_on_population).get_lower();
				}
			} else if (auto const leak_mechanism = dynamic_cast<MechanismLeak const*>(&mechanism);
			           leak_mechanism) {
				auto const& mechanism_parameter_space =
				    dynamic_cast<MechanismLeak::ParameterSpace const&>(
				        model_parameter_space.compartments
				            .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
				            .mechanisms.get(mechanism_on_compartment));

				for (size_t neuron_on_population = 0;
				     neuron_on_population < calibration_targets.size(); ++neuron_on_population) {
					for (size_t an = 0; an < atomic_neurons.size(); ++an) {
						// set leak potential on all circuits
						calibration_targets
						    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
						        compartment_on_neuron)]
						    .at(an)
						    .v_leak = mechanism_parameter_space.potential.at(neuron_on_population)
						                  .get_lower();
					}
					// set leak time constant on first circuit
					calibration_targets
					    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
					        compartment_on_neuron)]
					    .at(0)
					    .tau_membrane =
					    mechanism_parameter_space.time_constant.at(neuron_on_population)
					        .get_lower();
					mechanism_on_atomic_neuron_placement[grenade::common::CompartmentOnNeuron(
					    compartment_on_neuron)][mechanism_on_compartment]
					    .insert(0);
				}
			} else if (auto const fire_mechanism = dynamic_cast<MechanismFire const*>(&mechanism);
			           fire_mechanism) {
				auto const& mechanism_parameter_space =
				    dynamic_cast<MechanismFire::ParameterSpace const&>(
				        model_parameter_space.compartments
				            .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
				            .mechanisms.get(mechanism_on_compartment));

				for (size_t neuron_on_population = 0;
				     neuron_on_population < calibration_targets.size(); ++neuron_on_population) {
					// set threshold potential on first circuit
					calibration_targets
					    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
					        compartment_on_neuron)]
					    .at(0)
					    .v_threshold =
					    mechanism_parameter_space.threshold_potential.at(neuron_on_population)
					        .get_lower();

					// set reset potential on first circuit
					calibration_targets
					    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
					        compartment_on_neuron)]
					    .at(0)
					    .v_reset =
					    mechanism_parameter_space.reset_potential.at(neuron_on_population)
					        .get_lower();

					// set refractory period on first circuit
					calibration_targets
					    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
					        compartment_on_neuron)]
					    .at(0)
					    .refractory_period.refractory_time =
					    mechanism_parameter_space.refractory_time.at(neuron_on_population)
					        .get_lower();

					calibration_targets
					    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
					        compartment_on_neuron)]
					    .at(0)
					    .refractory_period.holdoff_time =
					    mechanism_parameter_space.holdoff_time.at(neuron_on_population).get_lower();
				}
				mechanism_on_atomic_neuron_placement[grenade::common::CompartmentOnNeuron(
				    compartment_on_neuron)][mechanism_on_compartment]
				    .insert(0);
			} else if (auto const synaptic_conductance_mechanism =
			               dynamic_cast<MechanismSynapticInputConductance const*>(&mechanism);
			           synaptic_conductance_mechanism) {
				auto const& mechanism_parameter_space =
				    dynamic_cast<MechanismSynapticInputConductance::ParameterSpace const&>(
				        model_parameter_space.compartments
				            .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
				            .mechanisms.get(mechanism_on_compartment));

				for (size_t an = 0; an < atomic_neurons.size(); ++an) {
					if (!calibrated_neuron.compartments
					         .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
					         .receptors.at(an)
					         .contains(grenade::common::ReceptorOnCompartment(
					             mechanism_on_compartment.value()))) {
						continue;
					}
					for (size_t neuron_on_population = 0;
					     neuron_on_population < calibration_targets.size();
					     ++neuron_on_population) {
						CalibratedNeuron::ParameterSpace::CalibrationTarget::CobaSynapticInput
						    coba_synaptic_input;
						coba_synaptic_input.e_reference =
						    mechanism_parameter_space.e_reference.at(neuron_on_population)
						        .get_lower();
						coba_synaptic_input.e_reversal =
						    mechanism_parameter_space.e_reversal.at(neuron_on_population)
						        .get_lower();
						coba_synaptic_input.i_synin_gm =
						    mechanism_parameter_space.i_synin_gm.at(neuron_on_population)
						        .get_lower();
						coba_synaptic_input.synapse_dac_bias =
						    mechanism_parameter_space.synapse_dac_bias.at(neuron_on_population)
						        .get_lower();
						coba_synaptic_input.tau_syn =
						    mechanism_parameter_space.time_constant.at(neuron_on_population)
						        .get_lower();
						if (synaptic_conductance_mechanism->receptor_type ==
						    MechanismSynapticInputConductance::ReceptorType::excitatory) {
							calibration_targets
							    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
							        compartment_on_neuron)]
							    .at(an)
							    .synaptic_input_excitatory = coba_synaptic_input;
						} else {
							calibration_targets
							    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
							        compartment_on_neuron)]
							    .at(an)
							    .synaptic_input_inhibitory = coba_synaptic_input;
						}
					}
				}
			} else if (auto const synaptic_current_mechanism =
			               dynamic_cast<MechanismSynapticInputCurrent const*>(&mechanism);
			           synaptic_current_mechanism) {
				auto const& mechanism_parameter_space =
				    dynamic_cast<MechanismSynapticInputCurrent::ParameterSpace const&>(
				        model_parameter_space.compartments
				            .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
				            .mechanisms.get(mechanism_on_compartment));

				for (size_t an = 0; an < atomic_neurons.size(); ++an) {
					if (!calibrated_neuron.compartments
					         .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
					         .receptors.at(an)
					         .contains(grenade::common::ReceptorOnCompartment(
					             mechanism_on_compartment.value()))) {
						continue;
					}
					for (size_t neuron_on_population = 0;
					     neuron_on_population < calibration_targets.size();
					     ++neuron_on_population) {
						CalibratedNeuron::ParameterSpace::CalibrationTarget::CubaSynapticInput
						    cuba_synaptic_input;
						cuba_synaptic_input.i_synin_gm =
						    mechanism_parameter_space.i_synin_gm.at(neuron_on_population)
						        .get_lower();
						cuba_synaptic_input.synapse_dac_bias =
						    mechanism_parameter_space.synapse_dac_bias.at(neuron_on_population)
						        .get_lower();
						cuba_synaptic_input.tau_syn =
						    mechanism_parameter_space.time_constant.at(neuron_on_population)
						        .get_lower();
						if (synaptic_current_mechanism->receptor_type ==
						    MechanismSynapticInputCurrent::ReceptorType::excitatory) {
							calibration_targets
							    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
							        compartment_on_neuron)]
							    .at(an)
							    .synaptic_input_excitatory = cuba_synaptic_input;
						} else {
							calibration_targets
							    .at(neuron_on_population)[grenade::common::CompartmentOnNeuron(
							        compartment_on_neuron)]
							    .at(an)
							    .synaptic_input_inhibitory = cuba_synaptic_input;
						}
					}
				}
			}
		}
	}

	// set multi-compartment switches and tau_icc from coordinate system and model
	// compartment connections
	std::set<CompartmentConnectionOnNeuron> unplaced_compartment_connections(
	    model_neuron.compartment_connections().begin(),
	    model_neuron.compartment_connections().end());

	for (auto const& [compartment_on_neuron, atomic_neurons] :
	     logical_neuron_compartments.get_compartments()) {
		size_t an = 0;
		for (auto const& atomic_neuron : atomic_neurons) {
			auto const unplaced_neuron_circuit = local_placement.coordinate_system.get_config(
			    atomic_neuron.toNeuronColumnOnLogicalNeuron().value(),
			    atomic_neuron.toNeuronRowOnLogicalNeuron().value());

			std::optional<CompartmentConnectionOnNeuron> placed_compartment_connection;
			if (unplaced_neuron_circuit.switch_circuit_shared_conductance) {
				// Find connnection in model which corresponds to the given resistor
				auto const connected_neuron_circuits =
				    local_placement.coordinate_system.connected_shared_conductance(
				        atomic_neuron.toNeuronColumnOnLogicalNeuron().value(),
				        atomic_neuron.toNeuronRowOnLogicalNeuron().value());
				if (connected_neuron_circuits.size() != 1) {
					throw std::runtime_error(
					    "Neuron circuit with local conductance is connected to more "
					    "than one other neuron circuit via shared line.");
				}
				auto const& connected_neuron_circuit = connected_neuron_circuits.at(0);
				auto const& connected_compartment =
				    local_placement.coordinate_system.get_compartment(
				        connected_neuron_circuit.first, connected_neuron_circuit.second);
				if (!connected_compartment) {
					throw std::runtime_error("Connected neuron circuit doesn't "
					                         "correspond to a compartment.");
				}
				for (auto const& compartment_connection : unplaced_compartment_connections) {
					auto const source = model_neuron.source(compartment_connection);
					auto const target = model_neuron.target(compartment_connection);
					if (source != compartment_on_neuron && target != compartment_on_neuron) {
						continue;
					}
					if (source != connected_compartment && target != connected_compartment) {
						continue;
					}
					placed_compartment_connection = compartment_connection;
					break;
				}
				if (!placed_compartment_connection) {
					throw std::runtime_error("No compartment connection found in model neuron "
					                         "matching placement result.");
				}
				unplaced_compartment_connections.erase(*placed_compartment_connection);
			}

			for (size_t neuron_on_population = 0; neuron_on_population < calibration_targets.size();
			     ++neuron_on_population) {
				CalibratedNeuron::ParameterSpace::CalibrationTarget::InterAtomicNeuronConnectivity
				    inter_atomic_neuron_connectivity;

				// set switches
				inter_atomic_neuron_connectivity.connect_vertical =
				    unplaced_neuron_circuit.switch_top_bottom;
				inter_atomic_neuron_connectivity.connect_right =
				    unplaced_neuron_circuit.switch_right;
				inter_atomic_neuron_connectivity.connect_soma_right =
				    unplaced_neuron_circuit.switch_shared_right;
				inter_atomic_neuron_connectivity.connect_soma =
				    unplaced_neuron_circuit.switch_circuit_shared;
				// why not get parameterization?
				// set conductance
				if (unplaced_neuron_circuit.switch_circuit_shared_conductance) {
					inter_atomic_neuron_connectivity.tau_icc =
					    dynamic_cast<CompartmentConnectionConductance::ParameterSpace const&>(
					        model_parameter_space.compartment_connections.get(
					            placed_compartment_connection.value()))
					        .time_constant.at(neuron_on_population)
					        .get_lower();
				}

				calibration_targets.at(neuron_on_population)
				    .at(grenade::common::CompartmentOnNeuron(compartment_on_neuron))
				    .at(an)
				    .inter_atomic_neuron_connectivity = inter_atomic_neuron_connectivity;
			}
			++an;
		}
	}

	return CalibratedNeuron::ParameterSpace(
	    std::move(calibration_targets), std::move(membrane_capacitances));
}

void MulticompartmentNeuronRewrite::replace_vertex(
    grenade::common::VertexOnTopology vertex_on_topology,
    grenade::common::Population const& model_population,
    std::map<
        grenade::common::CompartmentOnNeuron,
        std::map<MechanismOnCompartment, std::set<size_t>>> const&
        mechanism_on_atomic_neuron_placement,
    CalibratedNeuron&& calibrated_neuron,
    CalibratedNeuron::ParameterSpace&& calibrated_neuron_parameter_space,
    MulticompartmentNeuronMapping&& mapping) const
{
	// construct calibrated neuron population and add to topology
	grenade::common::Population calibrated_neuron_population(
	    std::move(calibrated_neuron), model_population.get_shape(),
	    std::move(calibrated_neuron_parameter_space), model_population.get_time_domain(),
	    model_population.get_execution_instance_on_executor());

	auto const calibrated_neuron_on_topology =
	    get_topology().add_vertex(std::move(calibrated_neuron_population));

	// add edges present in reference topology
	std::vector<typename grenade::common::LinkedTopology::LinkGraph::VertexDescriptor> old_links;
	for (auto const& link : get_topology().inter_graph_hyper_edges_by_linked(vertex_on_topology)) {
		auto const& references = get_topology().references(link);
		assert(references.size() == 1);
		old_links.push_back(references.at(0));
	}
	auto const model_population_input_ports = model_population.get_input_ports();
	auto const model_population_output_ports = model_population.get_output_ports();
	// create new in-edges
	std::vector<std::tuple<
	    grenade::common::VertexOnTopology, grenade::common::VertexOnTopology,
	    grenade::common::Edge>>
	    new_in_edges;
	for (auto const in_edge_descriptor : get_topology().in_edges(vertex_on_topology)) {
		auto const& in_edge = get_topology().get(in_edge_descriptor);
		if (model_population_input_ports.at(in_edge.port_on_target).get_type() != SynapticInput()) {
			throw std::runtime_error("Input port type not implemented.");
		}
		// find mechanism on compartment dimension
		auto const model_population_input_port_dimension_units =
		    model_population_input_ports.at(in_edge.port_on_target)
		        .get_channels()
		        .get_dimension_units();
		size_t const mechanism_dimension = std::distance(
		    model_population_input_port_dimension_units.begin(),
		    std::find(
		        model_population_input_port_dimension_units.begin(),
		        model_population_input_port_dimension_units.end(),
		        MechanismOnCompartmentDimensionUnit()));
		if (mechanism_dimension == model_population_input_port_dimension_units.size()) {
			throw std::runtime_error("Model neuron input port doesn't feature a mechanism on "
			                         "compartment dimension.");
		}
		// replace mechanism on compartment dimension unit by receptor on compartment
		auto dimension_units_on_target = in_edge.get_channels_on_target().get_dimension_units();
		dimension_units_on_target.at(mechanism_dimension) =
		    grenade::common::ReceptorOnCompartmentDimensionUnit();
		auto channels_on_target = in_edge.get_channels_on_target().copy();
		channels_on_target->set_dimension_units(std::move(dimension_units_on_target));
		new_in_edges.push_back(std::make_tuple(
		    get_topology().source(in_edge_descriptor), calibrated_neuron_on_topology,
		    grenade::common::Edge(
		        in_edge.get_channels_on_source(), std::move(*channels_on_target),
		        in_edge.port_on_source, 0)));
	}
	// create new out-edges
	std::vector<std::tuple<
	    grenade::common::VertexOnTopology, grenade::common::VertexOnTopology,
	    grenade::common::Edge>>
	    new_out_edges;
	for (auto const out_edge_descriptor : get_topology().out_edges(vertex_on_topology)) {
		auto const& out_edge = get_topology().get(out_edge_descriptor);
		// find mechanism on compartment dimension
		auto const model_population_output_port_dimension_units =
		    model_population_output_ports.at(out_edge.port_on_source)
		        .get_channels()
		        .get_dimension_units();
		size_t const mechanism_dimension = std::distance(
		    model_population_output_port_dimension_units.begin(),
		    std::find(
		        model_population_output_port_dimension_units.begin(),
		        model_population_output_port_dimension_units.end(),
		        MechanismOnCompartmentDimensionUnit()));
		if (mechanism_dimension == model_population_output_port_dimension_units.size()) {
			throw std::runtime_error("Model neuron output port doesn't feature a mechanism on "
			                         "compartment dimension.");
		}
		// translate mechanism on compartment dimension unit
		std::unique_ptr<grenade::common::MultiIndexSequence> channels_on_source;
		size_t port_on_source = 0;
		if (model_population_output_ports.at(out_edge.port_on_source).get_type() == Spike()) {
			// remove mechanism on compartment dimension unit
			std::set<size_t> dimensions;
			for (size_t d = 0; d < out_edge.get_channels_on_source().dimensionality(); ++d) {
				dimensions.insert(d);
			}
			dimensions.erase(mechanism_dimension);
			channels_on_source = out_edge.get_channels_on_source().projection(dimensions);
			port_on_source = 0;
		} else if (
		    model_population_output_ports.at(out_edge.port_on_source).get_type() ==
		    AnalogObservable()) {
			// find compartment on neuron dimension
			size_t const compartment_dimension = std::distance(
			    model_population_output_port_dimension_units.begin(),
			    std::find(
			        model_population_output_port_dimension_units.begin(),
			        model_population_output_port_dimension_units.end(),
			        grenade::common::CompartmentOnNeuronDimensionUnit()));
			if (compartment_dimension == model_population_output_port_dimension_units.size()) {
				throw std::runtime_error(
				    "Model neuron output port doesn't feature a compartment on "
				    "cell dimension.");
			}
			// use first circuit where mechanism is present
			auto channels_on_source_elements = out_edge.get_channels_on_source().get_elements();
			for (auto& element : channels_on_source_elements) {
				auto const& atomic_neurons_on_compartment =
				    mechanism_on_atomic_neuron_placement
				        .at(grenade::common::CompartmentOnNeuron(
				            element.value.at(compartment_dimension)))
				        .at(MechanismOnCompartment(element.value.at(mechanism_dimension)));
				assert(atomic_neurons_on_compartment.size() >= 1);
				element.value.at(mechanism_dimension) = *atomic_neurons_on_compartment.begin();
			}
			// replace mechanism with atomic neuron on compartment dimension unit
			auto dimension_units_on_source =
			    out_edge.get_channels_on_source().get_dimension_units();
			dimension_units_on_source.at(mechanism_dimension) =
			    AtomicNeuronOnCompartmentDimensionUnit();
			channels_on_source =
			    grenade::common::ListMultiIndexSequence(
			        std::move(channels_on_source_elements), std::move(dimension_units_on_source))
			        .move();
			port_on_source = 1;
		} else {
			throw std::runtime_error("Input port type not implemented.");
		}
		assert(channels_on_source);
		new_out_edges.push_back(std::make_tuple(
		    calibrated_neuron_on_topology, get_topology().target(out_edge_descriptor),
		    grenade::common::Edge(
		        *channels_on_source, out_edge.get_channels_on_target(), port_on_source,
		        out_edge.port_on_target)));
	}
	get_topology().clear_vertex(vertex_on_topology);
	// add new in-edges
	for (auto const& [source, target, edge] : new_in_edges) {
		get_topology().add_edge(source, target, edge);
	}
	// add new out-edges
	for (auto const& [source, target, edge] : new_out_edges) {
		get_topology().add_edge(source, target, edge);
	}
	get_topology().remove_vertex(vertex_on_topology);

	get_topology().add_inter_graph_hyper_edge(
	    {calibrated_neuron_on_topology}, {old_links.at(0)}, std::move(mapping));
}

void MulticompartmentNeuronRewrite::operator()() const
{
	// create copy of vertex descriptors because they are modified (invalidating iterators)
	std::vector<grenade::common::VertexOnTopology> vertices(
	    get_topology().vertices().begin(), get_topology().vertices().end());
	// iterate over all vertices, performing rewrite when encountering unplaced multi-compartment
	// neuron
	for (auto const& vertex_on_topology : vertices) {
		if (auto const model_population = dynamic_cast<grenade::common::Population const*>(
		        &get_topology().get(vertex_on_topology));
		    model_population) {
			if (auto const model_neuron =
			        dynamic_cast<Neuron const*>(&model_population->get_cell());
			    model_neuron) {
				auto const& model_parameter_space = dynamic_cast<Neuron::ParameterSpace const&>(
				    model_population->get_parameter_space());

				// construct environment
				auto const environment =
				    construct_environment(vertex_on_topology, *model_population);

				// perform local placement
				CoordinateSystem coordinate_system;
				ResourceManager resource_manager;
				resource_manager.add_config(*model_neuron, model_parameter_space, environment);
				assert(m_placement_algorithm);
				m_placement_algorithm->reset();
				auto tmp_local_placement =
				    m_placement_algorithm->run(coordinate_system, *model_neuron, resource_manager);

				// remove unused circuits on the left (this is done internally by
				// construct_logical_neuron_compartments; in order to later track unplaced neuron
				// circuit and logical neuron circuits, we remove them also here).
				tmp_local_placement.coordinate_system.align_left();
				auto const local_placement = tmp_local_placement;
				auto logical_neuron_compartments =
				    local_placement.coordinate_system.construct_logical_neuron_compartments();

				// check that all parameter space intervals only have one value
				check_is_single_operation_point(model_parameter_space);

				// construct CalibratedNeuron
				auto
				    [calibrated_neuron, mechanism_on_atomic_neuron_placement,
				     mechanism_readout_placement] =
				        construct_calibrated_neuron(
				            logical_neuron_compartments, *model_neuron, model_parameter_space,
				            environment);

				// construct CalibratedNeuron::ParameterSpace
				auto calibrated_neuron_parameter_space =
				    construct_calibrated_neuron_parameter_space(
				        calibrated_neuron, logical_neuron_compartments, local_placement,
				        *model_neuron, model_parameter_space, mechanism_on_atomic_neuron_placement);

				// construct mapping
				auto mapping = construct_mapping(
				    *model_neuron, model_population->size(), logical_neuron_compartments,
				    mechanism_readout_placement);

				// replace unplaced neuron with locally-placed calibrated neuron population in
				// topology
				replace_vertex(
				    vertex_on_topology, *model_population, mechanism_on_atomic_neuron_placement,
				    std::move(calibrated_neuron), std::move(calibrated_neuron_parameter_space),
				    std::move(mapping));
			}
		}
	}
}

} // namespace grenade::vx::network::abstract
