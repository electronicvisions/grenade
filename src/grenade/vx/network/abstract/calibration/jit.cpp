#include "grenade/vx/network/abstract/calibration/jit.h"

#include "ccalix/spiking_calib_options.h"
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/population.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/network/abstract/calibration/fixture.h"
#include "grenade/vx/network/abstract/mapping/calibrated_neuron.h"
#include "grenade/vx/network/abstract/population_cell/calibrated.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/variant.h"
#include "hxcomm/vx/connection_from_env.h"
#include "lola/vx/v3/neuron.h"
#include "pyhxcomm/vx/connection_handle.h"
#include <Python.h>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

namespace grenade::vx::network::abstract {

void JITCalibration::operator()(
    grenade::common::LinkedTopology& topology,
    grenade::vx::execution::JITGraphExecutor& executor) const
{
	using namespace grenade::common;
	// start Python interpreter if there is not already one running
	std::unique_ptr<pybind11::scoped_interpreter> python_interpreter;
	if (Py_IsInitialized() == 0) {
		python_interpreter = std::make_unique<pybind11::scoped_interpreter>();
	}

	auto pycalix = pybind11::module_::import("calix");
	auto pycalix_spiking = pybind11::module_::import("calix.spiking");
	auto calibrate_func = pycalix.attr("calibrate");

	// collect calibration targets
	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::map<
	        common::ChipOnConnection, std::map<
	                                      halco::hicann_dls::vx::v3::AtomicNeuronOnDLS,
	                                      CalibratedNeuron::ParameterSpace::CalibrationTarget>>>
	    calibration_targets;
	for (auto const& inter_graph_hyper_edge_descriptor : topology.inter_graph_hyper_edges()) {
		auto inter_graph_hyper_edge = topology.get(inter_graph_hyper_edge_descriptor).copy();
		if (auto const calibrated_ptr =
		        dynamic_cast<CalibratedNeuronMapping const*>(&(*inter_graph_hyper_edge));
		    calibrated_ptr) {
			auto const model_vertex_descriptor =
			    topology.references(inter_graph_hyper_edge_descriptor).at(0);
			auto const& calibrated_neuron = dynamic_cast<CalibratedNeuron const&>(
			    dynamic_cast<grenade::common::Population const&>(
			        topology.get_reference().get(model_vertex_descriptor))
			        .get_cell());
			auto const& parameter_space = dynamic_cast<CalibratedNeuron::ParameterSpace const&>(
			    dynamic_cast<grenade::common::Population const&>(
			        topology.get_reference().get(model_vertex_descriptor))
			        .get_parameter_space());
			std::set<ExecutionInstanceOnExecutor> mapped_execution_instance_on_executors;
			std::vector<std::reference_wrapper<signal_flow::vertex::NeuronView const>>
			    mapped_vertices;
			for (auto const& mapped_vertex_descriptor :
			     topology.links(inter_graph_hyper_edge_descriptor)) {
				auto const& mapped_vertex = dynamic_cast<signal_flow::vertex::NeuronView const&>(
				    topology.get(mapped_vertex_descriptor));
				mapped_vertices.emplace_back(mapped_vertex);
				mapped_execution_instance_on_executors.insert(
				    mapped_vertex.get_execution_instance_on_executor().value());
			}
			assert(mapped_execution_instance_on_executors.size() == 1);
			auto const& execution_instance_on_executor =
			    *mapped_execution_instance_on_executors.begin();
			for (auto const& [cell_on_population, anchor] : calibrated_ptr->anchors) {
				std::optional<common::ChipOnConnection> chip_on_connection;
				for (auto const& [_, mapped_vertex_index] : anchor.first) {
					auto const& local_chip_on_connection =
					    mapped_vertices.at(mapped_vertex_index).get().chip_on_connection;
					if (chip_on_connection &&
					    local_chip_on_connection != chip_on_connection.value()) {
						throw std::runtime_error("ChipOnConnection information of neuron views to "
						                         "single model neuron don't match.");
					}
					chip_on_connection = local_chip_on_connection;
				}
				auto logical_neuron_compartments = halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
				                                       calibrated_neuron.shape, anchor.second)
				                                       .get_placed_compartments();
				for (auto const& [compartment_on_neuron, atomic_neurons] :
				     logical_neuron_compartments) {
					auto const& local_calibration_targets =
					    parameter_space.calibration_targets.at(cell_on_population)
					        .at(CompartmentOnNeuron(compartment_on_neuron.value()));
					for (size_t an = 0; an < atomic_neurons.size(); ++an) {
						calibration_targets[execution_instance_on_executor]
						                   [chip_on_connection.value()][atomic_neurons.at(an)] =
						                       local_calibration_targets.at(an);
					}
				}
			}
		}
		topology.set(inter_graph_hyper_edge_descriptor, std::move(*inter_graph_hyper_edge));
	}

	FixtureCalibration fixture_calibration;

	// move connections to Python handles
	std::map<grenade::common::ConnectionOnExecutor, std::vector<bool>> enable_differential_config;
	std::map<grenade::common::ConnectionOnExecutor, pyhxcomm::vx::ConnectionHandle>
	    pyhxcomm_connections;
	std::map<grenade::common::ConnectionOnExecutor, std::vector<common::ChipOnConnection>>
	    chips_on_connections;
	std::map<grenade::common::ConnectionOnExecutor, execution::backend::StatefulConnection>
	    stateful_connections;
	std::map<grenade::common::ConnectionOnExecutor, execution::backend::InitializedConnection>
	    initialized_connections;
	stateful_connections = executor.release_connections();
	for (auto& [connection_on_executor, connection] : stateful_connections) {
		chips_on_connections.emplace(connection_on_executor, connection.get_chips_on_connection());
		initialized_connections.emplace(connection_on_executor, connection.release());
		enable_differential_config[connection_on_executor] =
		    connection.get_enable_differential_config();
		pyhxcomm_connections.emplace(
		    connection_on_executor,
		    std::visit(
		        [](auto&& ic) -> pyhxcomm::vx::ConnectionHandle {
			        return pyhxcomm::Handle<std::decay_t<decltype(ic)>>{std::move(ic)};
		        },
		        initialized_connections.at(connection_on_executor).release()));
	}

	auto pycalix_common = pybind11::module_::import("calix.common");
	std::map<
	    grenade::common::ConnectionOnExecutor, std::map<common::ChipOnConnection, pybind11::object>>
	    calix_stateful_connections;
	for (auto& [connection_on_executor, pyhxcomm_connection] : pyhxcomm_connections) {
		for (size_t c = 0;
		     auto const& chip_on_connection : chips_on_connections.at(connection_on_executor)) {
			std::visit(
			    [&](auto& pc) {
				    calix_stateful_connections[connection_on_executor].emplace(
				        chip_on_connection,
				        pycalix_common.attr("base").attr("StatefulConnection")(
				            pybind11::cast(pc, pybind11::return_value_policy::reference), c));
			    },
			    pyhxcomm_connection);
			++c;
		}
	}

	// perform calibration
	for (auto const& [execution_instance_on_executor, calibration_targets_on_connection] :
	     calibration_targets) {
		for (auto const& [chip_on_connection, calibration_target] :
		     calibration_targets_on_connection) {
			auto spiking_calib_target = target;

			// fill NeuronCalibTarget
			std::vector<std::set<haldls::vx::v3::CapMemCell::Value>> i_synin_gm(2);
			std::vector<double> e_rev_E;
			std::vector<double> e_rev_I;
			std::set<haldls::vx::v3::CapMemCell::Value> synapse_dac_bias;

			for (auto const& [atomic_neuron, target] : calibration_target) {
				spiking_calib_target.neuron_target.membrane_capacitance[atomic_neuron] =
				    target.membrane_capacitance_during_calibration
				        ? *target.membrane_capacitance_during_calibration
				        : target.membrane_capacitance;

				spiking_calib_target.neuron_target.leak[atomic_neuron] = target.v_leak;
				if (target.tau_membrane) {
					spiking_calib_target.neuron_target.tau_mem[atomic_neuron] =
					    *target.tau_membrane;
				}

				spiking_calib_target.neuron_target.reset[atomic_neuron] = target.v_reset;

				if (target.v_threshold) {
					spiking_calib_target.neuron_target.threshold[atomic_neuron] =
					    *target.v_threshold;
				}

				spiking_calib_target.neuron_target.refractory_time[atomic_neuron] =
				    target.refractory_period.refractory_time;
				spiking_calib_target.neuron_target.holdoff_time[atomic_neuron] =
				    target.refractory_period.holdoff_time;

				std::visit(
				    hate::overloaded{
				        [&](CalibratedNeuron::ParameterSpace::CalibrationTarget::
				                CubaSynapticInput const& target) {
					        spiking_calib_target.neuron_target.tau_syn
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::excitatory] =
					            target.tau_syn;
					        i_synin_gm.at(0).insert(target.i_synin_gm);
					        spiking_calib_target.neuron_target.coba_synin.e_coba_reversal
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::excitatory] =
					            std::nullopt;
					        spiking_calib_target.neuron_target.coba_synin.e_coba_reference
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::excitatory] =
					            std::nullopt;
					        e_rev_E.push_back(std::numeric_limits<double>::infinity());
					        synapse_dac_bias.insert(target.synapse_dac_bias);
				        },
				        [&](CalibratedNeuron::ParameterSpace::CalibrationTarget::
				                CobaSynapticInput const& target) {
					        spiking_calib_target.neuron_target.tau_syn
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::excitatory] =
					            target.tau_syn;
					        i_synin_gm.at(0).insert(target.i_synin_gm);
					        spiking_calib_target.neuron_target.coba_synin.e_coba_reversal
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::excitatory] =
					            target.e_reversal;
					        spiking_calib_target.neuron_target.coba_synin.e_coba_reference
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::excitatory] =
					            target.e_reference;
					        synapse_dac_bias.insert(target.synapse_dac_bias);
				        },
				        [](CalibratedNeuron::ParameterSpace::CalibrationTarget::
				               DisabledSynapticInput const&) {}},
				    target.synaptic_input_excitatory);

				std::visit(
				    hate::overloaded{
				        [&](CalibratedNeuron::ParameterSpace::CalibrationTarget::
				                CubaSynapticInput const& target) {
					        spiking_calib_target.neuron_target.tau_syn
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::inhibitory] =
					            target.tau_syn;
					        i_synin_gm.at(1).insert(target.i_synin_gm);
					        spiking_calib_target.neuron_target.coba_synin.e_coba_reversal
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::inhibitory] =
					            std::nullopt;
					        spiking_calib_target.neuron_target.coba_synin.e_coba_reference
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::inhibitory] =
					            std::nullopt;
					        e_rev_I.push_back(-std::numeric_limits<double>::infinity());
					        synapse_dac_bias.insert(target.synapse_dac_bias);
				        },
				        [&](CalibratedNeuron::ParameterSpace::CalibrationTarget::
				                CobaSynapticInput const& target) {
					        spiking_calib_target.neuron_target.tau_syn
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::inhibitory] =
					            target.tau_syn;
					        i_synin_gm.at(1).insert(target.i_synin_gm);
					        spiking_calib_target.neuron_target.coba_synin.e_coba_reversal
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::inhibitory] =
					            target.e_reversal;
					        spiking_calib_target.neuron_target.coba_synin.e_coba_reference
					            [atomic_neuron]
					            [halco::hicann_dls::vx::v3::SynapticInputOnNeuron::inhibitory] =
					            target.e_reference;
					        synapse_dac_bias.insert(target.synapse_dac_bias);
				        },
				        [](CalibratedNeuron::ParameterSpace::CalibrationTarget::
				               DisabledSynapticInput const&) {}},
				    target.synaptic_input_inhibitory);
			}

			if (synapse_dac_bias.size() > 1) {
				throw std::runtime_error(
				    "synapse_dac_bias is required to be equal for all neurons on one chip.");
			}
			if (i_synin_gm.at(0).size() > 1 || i_synin_gm.at(1).size() > 1) {
				throw std::runtime_error(
				    "i_synin_gm is required to be equal for all neurons on one chip.");
			}
			// if synaptic input is not used calibrate default
			if (i_synin_gm.at(0).empty()) {
				i_synin_gm.at(0).insert(lola::vx::v3::AtomicNeuron::AnalogValue(500));
			}
			if (i_synin_gm.at(1).empty()) {
				i_synin_gm.at(1).insert(lola::vx::v3::AtomicNeuron::AnalogValue(500));
			}

			spiking_calib_target.neuron_target.cuba_synin =
			    ccalix::NeuronCalibTarget::CalibratedCubaSynapticInput();
			std::get<ccalix::NeuronCalibTarget::CalibratedCubaSynapticInput>(
			    spiking_calib_target.neuron_target.cuba_synin)
			    .i_synin_gm[halco::hicann_dls::vx::v3::SynapticInputOnNeuron::excitatory] =
			    *i_synin_gm.at(0).begin();
			std::get<ccalix::NeuronCalibTarget::CalibratedCubaSynapticInput>(
			    spiking_calib_target.neuron_target.cuba_synin)
			    .i_synin_gm[halco::hicann_dls::vx::v3::SynapticInputOnNeuron::inhibitory] =
			    *i_synin_gm.at(1).begin();
			if (!synapse_dac_bias.empty()) {
				spiking_calib_target.neuron_target.synapse_dac_bias = *synapse_dac_bias.begin();
			}

			auto pyspiking_calib_target = pycalix_spiking.attr("SpikingCalibTarget")();
			pyspiking_calib_target.cast<ccalix::SpikingCalibTarget&>() = spiking_calib_target;
			// TODO: deduplicate, only needed because of cache paths
			if (cache_paths) {
				auto calibration_result = calibrate_func(
				    pyspiking_calib_target, options, *cache_paths, false,
				    calix_stateful_connections
				        .at(execution_instance_on_executor.connection_on_executor)
				        .at(chip_on_connection));
				auto pystadls = pybind11::module_::import("pystadls_vx_v3");
				auto calibration_result_dumper = pystadls.attr("PlaybackProgramBuilderDumper")();
				calibration_result.attr("apply")(calibration_result_dumper);
				fixture_calibration.chips[execution_instance_on_executor].emplace(
				    chip_on_connection,
				    stadls::vx::v3::convert_to_chip(
				        calibration_result_dumper
				            .cast<stadls::vx::v3::PlaybackProgramBuilderDumper&>()
				            .done()));
			} else {
				auto calibration_result = calibrate_func(
				    pyspiking_calib_target, options, pybind11::none(), false,
				    calix_stateful_connections
				        .at(execution_instance_on_executor.connection_on_executor)
				        .at(chip_on_connection));
				auto pystadls = pybind11::module_::import("pystadls_vx_v3");
				auto calibration_result_dumper = pystadls.attr("PlaybackProgramBuilderDumper")();
				calibration_result.attr("apply")(calibration_result_dumper);
				fixture_calibration.chips[execution_instance_on_executor].emplace(
				    chip_on_connection,
				    stadls::vx::v3::convert_to_chip(
				        calibration_result_dumper
				            .cast<stadls::vx::v3::PlaybackProgramBuilderDumper&>()
				            .done()));
			}
		}
	}

	// TODO: perform inter-compartment-conductance calibration here
	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::map<
	        halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, lola::vx::v3::AtomicNeuron::AnalogValue>>
	    inter_compartment_conductances;

	// perform fix-ups for multi-compartment connectivity and disabling of mechanisms
	for (auto const& inter_graph_hyper_edge_descriptor : topology.inter_graph_hyper_edges()) {
		auto inter_graph_hyper_edge = topology.get(inter_graph_hyper_edge_descriptor).copy();
		if (auto const calibrated_ptr =
		        dynamic_cast<CalibratedNeuronMapping const*>(&(*inter_graph_hyper_edge));
		    calibrated_ptr) {
			auto const model_vertex_descriptor =
			    topology.references(inter_graph_hyper_edge_descriptor).at(0);
			auto const& calibrated_neuron = dynamic_cast<CalibratedNeuron const&>(
			    dynamic_cast<grenade::common::Population const&>(
			        topology.get_reference().get(model_vertex_descriptor))
			        .get_cell());
			auto const& parameter_space = dynamic_cast<CalibratedNeuron::ParameterSpace const&>(
			    dynamic_cast<grenade::common::Population const&>(
			        topology.get_reference().get(model_vertex_descriptor))
			        .get_parameter_space());
			std::set<ExecutionInstanceOnExecutor> mapped_execution_instance_on_executors;
			std::vector<std::reference_wrapper<signal_flow::vertex::NeuronView const>>
			    mapped_vertices;
			for (auto const& mapped_vertex_descriptor :
			     topology.links(inter_graph_hyper_edge_descriptor)) {
				auto const& mapped_vertex = dynamic_cast<signal_flow::vertex::NeuronView const&>(
				    topology.get(mapped_vertex_descriptor));
				mapped_vertices.emplace_back(mapped_vertex);
				mapped_execution_instance_on_executors.insert(
				    mapped_vertex.get_execution_instance_on_executor().value());
			}
			assert(mapped_execution_instance_on_executors.size() == 1);
			auto const& execution_instance_on_executor =
			    *mapped_execution_instance_on_executors.begin();
			for (auto const& [cell_on_population, anchor] : calibrated_ptr->anchors) {
				auto logical_neuron_compartments = halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
				                                       calibrated_neuron.shape, anchor.second)
				                                       .get_placed_compartments();
				for (auto const& [compartment_on_neuron, atomic_neurons] :
				     logical_neuron_compartments) {
					auto const& local_calibration_targets =
					    parameter_space.calibration_targets.at(cell_on_population)
					        .at(CompartmentOnNeuron(compartment_on_neuron.value()));
					for (size_t an = 0; an < atomic_neurons.size(); ++an) {
						std::optional<common::ChipOnConnection> chip_on_connection;
						for (auto const& [_, mapped_vertex_index] : anchor.first) {
							auto const& local_chip_on_connection =
							    mapped_vertices.at(mapped_vertex_index).get().chip_on_connection;
							if (chip_on_connection &&
							    local_chip_on_connection != chip_on_connection.value()) {
								throw std::runtime_error(
								    "ChipOnConnection information of neuron views to "
								    "single model neuron don't match.");
							}
							chip_on_connection = local_chip_on_connection;
						}
						auto& atomic_neuron_config =
						    fixture_calibration.chips.at(execution_instance_on_executor)
						        .at(chip_on_connection.value())
						        .neuron_block.atomic_neurons.at(atomic_neurons.at(an));
						auto const& local_calibration_target = local_calibration_targets.at(an);
						// apply inter-compartment switches
						atomic_neuron_config.multicompartment.connect_soma =
						    local_calibration_target.inter_atomic_neuron_connectivity.connect_soma;
						atomic_neuron_config.multicompartment.connect_soma_right =
						    local_calibration_target.inter_atomic_neuron_connectivity
						        .connect_soma_right;
						atomic_neuron_config.multicompartment.connect_vertical =
						    local_calibration_target.inter_atomic_neuron_connectivity
						        .connect_vertical;
						atomic_neuron_config.multicompartment.connect_right =
						    local_calibration_target.inter_atomic_neuron_connectivity.connect_right;
						atomic_neuron_config.multicompartment.enable_conductance =
						    local_calibration_target.inter_atomic_neuron_connectivity.tau_icc
						        .has_value();
						// disable membrane time constant
						if (!local_calibration_target.tau_membrane) {
							atomic_neuron_config.leak.i_bias =
							    lola::vx::v3::AtomicNeuron::AnalogValue(0);
							atomic_neuron_config.leak.enable_division = true;
							atomic_neuron_config.leak.enable_multiplication = false;
						}
						// disable threshold
						if (!local_calibration_target.v_threshold) {
							atomic_neuron_config.threshold.enable = false;
						}
						if (local_calibration_target.membrane_capacitance_during_calibration) {
							// set requested membrane_capacitance
							atomic_neuron_config.membrane_capacitance.capacitance =
							    local_calibration_target.membrane_capacitance;
						}
					}
				}
			}
		}
		topology.set(inter_graph_hyper_edge_descriptor, std::move(*inter_graph_hyper_edge));
	}

	// move connection back into executor
	for (auto& [connection_on_executor, pyhxcomm_connection] : pyhxcomm_connections) {
		initialized_connections.at(connection_on_executor)
		    .capture(std::visit(
		        [](auto& c) -> hxcomm::vx::ConnectionVariant { return std::move(c.get()); },
		        pyhxcomm_connection));

		stateful_connections.at(connection_on_executor)
		    .capture(std::move(initialized_connections.at(connection_on_executor)));
		/*stateful_connections.at(connection_on_executor) = execution::backend::StatefulConnection(
		    std::move(initialized_connections.at(connection_on_executor)),
		    enable_differential_config.at(connection_on_executor));*/
	}
	executor.capture_connections(std::move(stateful_connections));

	fixture_calibration(topology, executor);
}

} // namespace grenade::vx::network::abstract
