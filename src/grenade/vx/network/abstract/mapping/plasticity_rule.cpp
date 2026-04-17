#include "grenade/vx/network/abstract/mapping/plasticity_rule.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/projection.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/mapping/locally_placed_neuron.h"
#include "grenade/vx/network/abstract/mapping/uncalibrated_synapse.h"
#include "grenade/vx/network/abstract/plasticity_rule.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"
#include "hate/variant.h"


namespace grenade::vx::network::abstract {

PlasticityRuleMapping::PlasticityRuleMapping() {}

bool PlasticityRuleMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (linked_vertex_descriptors.size() != 1) {
		return false;
	}
	auto const& mapped_vertex = topology.get(linked_vertex_descriptors.at(0));
	if (dynamic_cast<grenade::vx::signal_flow::vertex::PlasticityRule const*>(&mapped_vertex) ==
	    nullptr) {
		return false;
	}
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const recorder = dynamic_cast<grenade::vx::network::abstract::PlasticityRule const*>(
	        &topology.get_reference().get(reference_vertex_descriptors.at(0)));
	    recorder) {
		return true;
	}
	return false;
}

namespace {

template <typename T>
void extract_plasticity_rule_recording_data_per_synapse(
    std::map<size_t, PlasticityRule::Results::TimedData::EntryPerSynapse>& local_logical_data,
    grenade::common::VertexOnTopology const& descriptor,
    size_t projection_index,
    grenade::common::Topology const& partitioned_topology,
    size_t const batch_size,
    size_t const hardware_index,
    size_t const index,
    std::vector<common::TimedDataSequence<std::vector<T>>> const& local_hardware_data)
{
	if (!local_logical_data.contains(projection_index)) {
		std::vector<common::TimedDataSequence<std::vector<std::vector<T>>>>
		    logical_local_hardware_data(batch_size);
		for (size_t b = 0; b < logical_local_hardware_data.size(); ++b) {
			auto& batch = logical_local_hardware_data.at(b);
			batch.resize(local_hardware_data.at(b).size());
			auto const& connector = dynamic_cast<grenade::common::Projection const&>(
			                            partitioned_topology.get(descriptor))
			                            .get_connector();
			for (auto& sample : batch) {
				sample.data.resize(
				    connector.get_num_synapses(*connector.get_input_sequence()->cartesian_product(
				        *connector.get_output_sequence())));
			}
			for (size_t s = 0; s < local_hardware_data.at(b).size(); ++s) {
				batch.at(s).time = local_hardware_data.at(b).at(s).time;
			}
		}
		local_logical_data[projection_index] = logical_local_hardware_data;
	}
	auto& logical_local_hardware_data =
	    std::get<std::vector<common::TimedDataSequence<std::vector<std::vector<T>>>>>(
	        local_logical_data.at(projection_index));
	for (size_t b = 0; b < local_hardware_data.size(); ++b) {
		for (size_t s = 0; s < local_hardware_data.at(b).size(); ++s) {
			auto& local_logical_local_hardware_data = logical_local_hardware_data.at(b).at(s);
			auto const& local_local_hardware_data = local_hardware_data.at(b).at(s);
			local_logical_local_hardware_data.data.at(index).push_back(
			    local_local_hardware_data.data.at(hardware_index));
		}
	}
}

template <typename T>
void extract_plasticity_rule_recording_data_per_neuron(
    std::map<size_t, PlasticityRule::Results::TimedData::EntryPerNeuron>& local_logical_data,
    size_t population_index,
    size_t const batch_size,
    std::vector<std::map<
        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
        std::vector<std::pair<grenade::common::VertexOnTopology, size_t>>>> const&
        neuron_translation,
    std::vector<common::TimedDataSequence<std::vector<T>>> const& dd)
{
	if (!local_logical_data.contains(population_index)) {
		// dimension (outer to inner): batch, time, cell_on_population, compartment_on_neuron,
		// atomic_neuron_on_compartment
		std::vector<common::TimedDataSequence<std::vector<
		    std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, std::vector<T>>>>>
		    logical_dd(batch_size);
		for (size_t b = 0; b < logical_dd.size(); ++b) {
			auto& batch = logical_dd.at(b);
			batch.resize(dd.at(b).size());
			for (auto& sample : batch) {
				sample.data.resize(neuron_translation.size());
			}
			for (size_t s = 0; s < dd.at(b).size(); ++s) {
				batch.at(s).time = dd.at(b).at(s).time;
			}
		}
		local_logical_data[population_index] = logical_dd;
	}
	auto& logical_dd = std::get<std::vector<common::TimedDataSequence<std::vector<
	    std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, std::vector<T>>>>>>(
	    local_logical_data.at(population_index));
	for (size_t b = 0; b < dd.size(); ++b) {
		for (size_t s = 0; s < dd.at(b).size(); ++s) {
			auto& local_logical_dd = logical_dd.at(b).at(s);
			auto& local_dd = dd.at(b).at(s);
			for (size_t neuron = 0; neuron < neuron_translation.size(); ++neuron) {
				auto& neuron_data = local_logical_dd.data.at(neuron);
				for (auto const& [compartment, atomic_neuron_indices] :
				     neuron_translation.at(neuron)) {
					auto& compartment_data = neuron_data[compartment];
					for (auto const& [_, atomic_neuron_index] : atomic_neuron_indices) {
						compartment_data.push_back(local_dd.data.at(atomic_neuron_index));
					}
				}
			}
		}
	}
}

} // namespace

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
PlasticityRuleMapping::map_output_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        linked_vertex_output_data,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (linked_vertex_output_data.empty()) {
		throw std::invalid_argument("Mapping results not possible without mapped vertex results.");
	}
	auto const& partitioned_topology =
	    dynamic_cast<grenade::common::LinkedTopology const&>(topology.get_reference());

	auto const& partitioned_vertex = dynamic_cast<PlasticityRule const&>(
	    partitioned_topology.get(reference_vertex_descriptors.at(0)));

	if (!partitioned_vertex.recording) {
		std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ret;
		ret.push_back({});
		ret.back().emplace_back(nullptr);
		return ret;
	}

	size_t const batch_size = dynamic_cast<grenade::common::BatchedPortData const&>(
	                              linked_vertex_output_data.at(0).at(0).value().get())
	                              .batch_size();

	auto const& mapped_plasticity_results =
	    dynamic_cast<signal_flow::vertex::PlasticityRule::Results const&>(
	        linked_vertex_output_data.at(0).at(0).value().get());

	if (std::holds_alternative<signal_flow::vertex::PlasticityRule::RawRecordingData>(
	        mapped_plasticity_results.data)) {
		std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ret;
		ret.push_back({});
		ret.back().emplace_back(std::make_unique<PlasticityRule::Results>(
		    std::get<signal_flow::vertex::PlasticityRule::RawRecordingData>(
		        mapped_plasticity_results.data)));
		return ret;
	} else if (!std::holds_alternative<signal_flow::vertex::PlasticityRule::TimedRecordingData>(
	               mapped_plasticity_results.data)) {
		throw std::logic_error("Recording data type not implemented.");
	}

	auto const& mapped_plasticity_results_data =
	    std::get<signal_flow::vertex::PlasticityRule::TimedRecordingData>(
	        mapped_plasticity_results.data);

	PlasticityRule::Results::TimedData logical_data;
	logical_data.data_array = mapped_plasticity_results_data.data_array;

	// create location lookup for data_per_synapse map<VertexOnTopology, IndexOnVertexData>
	std::map<grenade::common::VertexOnTopology, size_t> data_per_synapse_lookup_table;
	std::map<grenade::common::VertexOnTopology, size_t> data_per_neuron_lookup_table;
	std::vector<grenade::common::VertexOnTopology> synapse_vertices;
	std::vector<grenade::common::VertexOnTopology> neuron_vertices;
	for (auto const& in_edge : topology.in_edges(linked_vertex_descriptors.at(0))) {
		auto const source = topology.source(in_edge);
		if (dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse const*>(
		        &topology.get(source)) == nullptr) {
			continue;
		}
		synapse_vertices.push_back(source);
		data_per_synapse_lookup_table[source] = topology.get(in_edge).port_on_target;
	}
	for (auto const& in_edge : topology.in_edges(linked_vertex_descriptors.at(0))) {
		auto const source = topology.source(in_edge);
		if (dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse const*>(
		        &topology.get(source)) == nullptr) {
			neuron_vertices.push_back(source);
			data_per_neuron_lookup_table[source] =
			    topology.get(in_edge).port_on_target - synapse_vertices.size();
			continue;
		}
	}

	auto const extract_observable_per_synapse = [&](auto const& type, auto const& name) {
		typedef typename std::decay_t<decltype(type)>::ElementType ElementType;
		for (auto const in_edge :
		     partitioned_topology.in_edges(reference_vertex_descriptors.at(0))) {
			auto const source = partitioned_topology.source(in_edge);
			size_t const projection_index = partitioned_topology.get(in_edge).port_on_target;
			if (auto const projection = dynamic_cast<grenade::common::Projection const*>(
			        &partitioned_topology.get(source));
			    projection != nullptr) {
				for (auto const& inter_topology_hyper_edge_descriptor :
				     topology.inter_graph_hyper_edges_by_reference(source)) {
					if (auto const uncalibrated_synapse_mapping =
					        dynamic_cast<UncalibratedSynapseMapping const*>(
					            &topology.get(inter_topology_hyper_edge_descriptor));
					    uncalibrated_synapse_mapping) {
						auto const& links = topology.links(inter_topology_hyper_edge_descriptor);
						for (
						    auto const& [synapse_on_partitioned_vertex, synapses_on_mapped_vertices] :
						    uncalibrated_synapse_mapping->translation) {
							for (size_t mapped_vertex_descriptor_index = 0;
							     mapped_vertex_descriptor_index <
							     synapses_on_mapped_vertices.size();
							     ++mapped_vertex_descriptor_index) {
								auto const& link_synapse_view_descriptor =
								    links.at(mapped_vertex_descriptor_index);

								for (auto const& synapse_on_mapped_vertex :
								     synapses_on_mapped_vertices.at(
								         mapped_vertex_descriptor_index)) {
									auto const& local_result = std::get<std::vector<
									    common::TimedDataSequence<std::vector<ElementType>>>>(
									    mapped_plasticity_results_data.data_per_synapse.at(name).at(
									        data_per_synapse_lookup_table.at(
									            link_synapse_view_descriptor)));
									extract_plasticity_rule_recording_data_per_synapse(
									    logical_data.data_per_synapse[name], source,
									    projection_index, partitioned_topology, batch_size,
									    synapse_on_mapped_vertex, synapse_on_partitioned_vertex,
									    local_result);
								}
							}
						}
					}
				}
			}
		}
	};

	auto const extract_observable_per_neuron = [&](auto const& type, auto const& name) {
		typedef typename std::decay_t<decltype(type)>::ElementType ElementType;
		for (auto const in_edge :
		     partitioned_topology.in_edges(reference_vertex_descriptors.at(0))) {
			auto const source = partitioned_topology.source(in_edge);
			size_t const population_index = partitioned_topology.get(in_edge).port_on_target -
			                                partitioned_vertex.get_projection_shapes().size();
			if (auto const population = dynamic_cast<grenade::common::Population const*>(
			        &partitioned_topology.get(source));
			    population != nullptr) {
				std::vector<common::TimedDataSequence<std::vector<ElementType>>> local_record;
				std::vector<std::map<
				    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
				    std::vector<std::pair<grenade::common::VertexOnTopology, size_t>>>>
				    neuron_translation;
				for (auto const& inter_topology_hyper_edge_descriptor :
				     topology.inter_graph_hyper_edges_by_reference(source)) {
					if (auto const locally_placed_neuron_mapping =
					        dynamic_cast<LocallyPlacedNeuronMapping const*>(
					            &topology.get(inter_topology_hyper_edge_descriptor));
					    locally_placed_neuron_mapping) {
						std::map<
						    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS,
						    std::pair<grenade::common::VertexOnTopology, size_t>>
						    mapped_neuron_translation;
						size_t neuron_view_offset = 0;
						for (auto const& mapped_vertex_descriptor :
						     topology.links(inter_topology_hyper_edge_descriptor)) {
							auto const& local_result = std::get<
							    std::vector<common::TimedDataSequence<std::vector<ElementType>>>>(
							    mapped_plasticity_results_data.data_per_neuron.at(name).at(
							        data_per_neuron_lookup_table.at(mapped_vertex_descriptor)));
							local_record.resize(local_result.size());
							for (size_t i = 0; i < local_record.size(); ++i) {
								local_record.at(i).resize(local_result.at(i).size());
								for (size_t s = 0; s < local_record.at(i).size(); ++s) {
									// TODO: make array over hemispheres
									local_record.at(i).at(s).time = local_result.at(i).at(s).time;
									local_record.at(i).at(s).data.resize(
									    population->get_shape().size());
									for (size_t c = 0; c < local_result.at(i).at(s).data.size();
									     ++c) {
										local_record.at(i).at(s).data.at(c + neuron_view_offset) =
										    local_result.at(i).at(s).data.at(c);
									}
								}
							}
							if (local_result.size() && local_result.at(0).size()) {
								neuron_view_offset += local_result.at(0).at(0).data.size();
							}
							auto const& mapped_vertex =
							    dynamic_cast<signal_flow::vertex::NeuronView const&>(
							        topology.get(mapped_vertex_descriptor));
							for (size_t c = 0; auto const& column : mapped_vertex.get_columns()) {
								mapped_neuron_translation.emplace(
								    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS(
								        column, mapped_vertex.row),
								    std::pair{mapped_vertex_descriptor, c});
								c++;
							}
						}
						for (auto const& [_, anchor] : locally_placed_neuron_mapping->anchors) {
							std::map<
							    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
							    std::vector<std::pair<grenade::common::VertexOnTopology, size_t>>>
							    local_neuron_translation;
							halco::hicann_dls::vx::v3::LogicalNeuronOnDLS logical_neuron_on_dls(
							    dynamic_cast<LocallyPlacedNeuron const&>(population->get_cell())
							        .shape,
							    anchor.second);
							for (auto const& [compartment_on_neuron, atomic_neurons] :
							     logical_neuron_on_dls.get_placed_compartments()) {
								for (auto const& atomic_neuron : atomic_neurons) {
									local_neuron_translation[compartment_on_neuron].push_back(
									    mapped_neuron_translation.at(atomic_neuron));
								}
							}

							neuron_translation.push_back(local_neuron_translation);
						}
					}
				}
				extract_plasticity_rule_recording_data_per_neuron(
				    logical_data.data_per_neuron[name], population_index, batch_size,
				    neuron_translation, local_record);
			}
		}
	};

	for (auto const& [name, obsv] :
	     std::get<PlasticityRule::TimedRecordingConfig>(*partitioned_vertex.recording)
	         .observables) {
		std::visit(
		    hate::overloaded{
		        [&](PlasticityRule::TimedRecordingConfig::ObservablePerSynapse const& observable) {
			        std::visit(
			            [&](auto const& type) { extract_observable_per_synapse(type, name); },
			            observable.type);
		        },
		        [&](PlasticityRule::TimedRecordingConfig::ObservablePerNeuron const& observable) {
			        std::visit(
			            [&](auto const& type) { extract_observable_per_neuron(type, name); },
			            observable.type);
		        },
		        [&](PlasticityRule::TimedRecordingConfig::ObservableArray const&) {
			        // handled above, no conversion needed
		        },
		        [&](auto const&) { throw std::logic_error("Observable type not implemented."); }},
		    obsv);
	}

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ret;
	ret.push_back({});
	ret.back().emplace_back(std::make_unique<PlasticityRule::Results>(std::move(logical_data)));
	return ret;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
PlasticityRuleMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        model_vertex_input_data,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::LinkedTopology const& topology) const
{
	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ret(1);
	ret.at(0).resize(topology.get(linked_vertex_descriptors.at(0)).get_input_ports().size());
	ret.at(0).at(ret.at(0).size() - 2) =
	    std::make_unique<signal_flow::vertex::PlasticityRule::Parameterization>(
	        dynamic_cast<PlasticityRule::Parameterization const&>(
	            model_vertex_input_data.at(0)
	                .at(model_vertex_input_data.at(0).size() - 2)
	                .value()
	                .get())
	            .kernel);

	auto const& model_plasticity_dynamics = dynamic_cast<PlasticityRule::Dynamics const&>(
	    model_vertex_input_data.at(0).at(model_vertex_input_data.at(0).size() - 1).value().get());
	ret.at(0).at(ret.at(0).size() - 1) =
	    std::make_unique<signal_flow::vertex::PlasticityRule::Dynamics>(
	        model_plasticity_dynamics.batch_size(), model_plasticity_dynamics.timer);
	return ret;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> PlasticityRuleMapping::copy() const
{
	return std::make_unique<PlasticityRuleMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> PlasticityRuleMapping::move()
{
	return std::make_unique<PlasticityRuleMapping>(std::move(*this));
}

std::ostream& PlasticityRuleMapping::print(std::ostream& os) const
{
	return os << "PlasticityRuleMapping()";
}

bool PlasticityRuleMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return InterTopologyHyperEdge::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
