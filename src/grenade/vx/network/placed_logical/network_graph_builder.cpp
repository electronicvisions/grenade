#include "grenade/vx/network/placed_logical/network_graph_builder.h"

#include "grenade/vx/network/placed_atomic/network_builder.h"
#include "grenade/vx/network/placed_logical/build_atomic_network.h"
#include "grenade/vx/network/placed_logical/build_connection_routing.h"
#include "grenade/vx/network/placed_logical/requires_routing.h"
#include "hate/math.h"
#include "hate/variant.h"
#include <map>
#include <log4cxx/logger.h>

namespace grenade::vx::network::placed_logical {

NetworkGraph build_network_graph(
    std::shared_ptr<Network> const& network, RoutingResult const& routing_result)
{
	assert(network);

	auto const atomic_network =
	    build_atomic_network(network, routing_result.connection_routing_result);

	NetworkGraph result;
	result.m_hardware_network_graph =
	    placed_atomic::build_network_graph(atomic_network, routing_result.atomic_routing_result);

	// construct hardware network
	network::placed_atomic::NetworkBuilder builder;

	// add populations, currently direct translation
	NetworkGraph::PopulationTranslation population_translation;
	NetworkGraph::NeuronTranslation neuron_translation;
	for (auto const& [descriptor, population] : network->populations) {
		auto const construct_population = [descriptor, &neuron_translation](Population const& pop) {
			network::placed_atomic::Population hardware_pop;
			hardware_pop.neurons.reserve(pop.get_atomic_neurons_size());
			hardware_pop.enable_record_spikes.resize(pop.get_atomic_neurons_size(), false);

			size_t index = 0;
			for (auto const& neuron : pop.neurons) {
				std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, std::vector<size_t>>
				    indices;
				for (auto const& [compartment_on_neuron, compartment] :
				     neuron.coordinate.get_placed_compartments()) {
					for (auto const& atomic_neuron : compartment) {
						hardware_pop.neurons.push_back(atomic_neuron);
						indices[compartment_on_neuron].push_back(index);
						index++;
					}
					auto const& spike_master =
					    neuron.compartments.at(compartment_on_neuron).spike_master;
					if (spike_master && spike_master->enable_record_spikes) {
						hardware_pop.enable_record_spikes.at(
						    index - compartment.size() + spike_master->neuron_on_compartment) =
						    true;
					}
				}
				neuron_translation[descriptor].push_back(indices);
			}
			return hardware_pop;
		};
		population_translation[descriptor] = std::visit(
		    hate::overloaded{
		        [&builder, construct_population](Population const& pop) {
			        return builder.add(construct_population(pop));
		        },
		        [&builder, &neuron_translation, descriptor](auto const& pop /* BackgroundSpikeSoucePopulation, ExternalPopulation stay the same */) {
			for (size_t i = 0; i < pop.size; ++i) {
				neuron_translation[descriptor].push_back({{halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron(), {i}}});
			}
			 return builder.add(pop);
			 }},
		    population);
	}

	// add MADC recording if present
	if (network->madc_recording) {
		network::placed_atomic::MADCRecording hardware_madc_recording;
		hardware_madc_recording.population =
		    population_translation.at(network->madc_recording->population);
		auto const& local_neuron_translation =
		    neuron_translation.at(network->madc_recording->population);
		hardware_madc_recording.index =
		    local_neuron_translation.at(network->madc_recording->neuron_on_population)
		        .at(network->madc_recording->compartment_on_neuron)
		        .at(network->madc_recording->atomic_neuron_on_compartment);
		hardware_madc_recording.source = network->madc_recording->source;
		builder.add(hardware_madc_recording);
	}

	// add CADC recording if present
	if (network->cadc_recording) {
		network::placed_atomic::CADCRecording hardware_cadc_recording;
		for (auto const& neuron : network->cadc_recording->neurons) {
			network::placed_atomic::CADCRecording::Neuron hardware_neuron;
			hardware_neuron.population = population_translation.at(neuron.population);
			hardware_neuron.index = neuron_translation.at(neuron.population)
			                            .at(neuron.neuron_on_population)
			                            .at(neuron.compartment_on_neuron)
			                            .at(neuron.atomic_neuron_on_compartment);
			hardware_neuron.source = neuron.source;
			hardware_cadc_recording.neurons.push_back(hardware_neuron);
		}
		builder.add(hardware_cadc_recording);
	}

	// add projections, distribute weight onto multiple synapses
	// we split the weight like: 123 -> 63 + 60
	// we distribute over all neurons in every compartment
	NetworkGraph::ProjectionTranslation projection_translation;
	std::map<ProjectionDescriptor, network::placed_atomic::ProjectionDescriptor>
	    projection_descriptor_translation;
	for (auto const& [descriptor, projection] : network->projections) {
		network::placed_atomic::Projection hardware_projection;
		hardware_projection.receptor_type = projection.receptor.type;
		hardware_projection.population_pre = population_translation.at(projection.population_pre);
		hardware_projection.population_post = population_translation.at(projection.population_post);
		auto const& population_pre = network->populations.at(projection.population_pre);
		std::vector<size_t> indices;
		if (!projection.connections.empty()) {
			auto const max_num_synapses =
			    std::max_element(
			        routing_result.connection_routing_result.at(descriptor).begin(),
			        routing_result.connection_routing_result.at(descriptor).end(),
			        [](auto const& a, auto const& b) {
				        return a.atomic_neurons_on_target_compartment.size() <
				               b.atomic_neurons_on_target_compartment.size();
			        })
			        ->atomic_neurons_on_target_compartment.size();
			for (size_t p = 0; p < max_num_synapses; ++p) {
				for (size_t i = 0; i < projection.connections.size(); ++i) {
					// only find new hardware synapse, if the current connection requires another
					// one
					if (routing_result.connection_routing_result.at(descriptor)
					        .at(i)
					        .atomic_neurons_on_target_compartment.size() <= p) {
						continue;
					}
					auto const& local_connection = projection.connections.at(i);
					auto const neuron_on_compartment =
					    routing_result.connection_routing_result.at(descriptor)
					        .at(i)
					        .atomic_neurons_on_target_compartment.at(p);
					size_t index_pre;
					if (std::holds_alternative<Population>(population_pre)) {
						auto const& pop = std::get<Population>(population_pre);
						auto const& spike_master =
						    pop.neurons.at(local_connection.index_pre.first)
						        .compartments.at(local_connection.index_pre.second)
						        .spike_master;
						assert(spike_master);
						index_pre = neuron_translation.at(projection.population_pre)
						                .at(local_connection.index_pre.first)
						                .at(local_connection.index_pre.second)
						                .at(spike_master->neuron_on_compartment);
					} else {
						index_pre = local_connection.index_pre.first;
						assert(
						    local_connection.index_pre.second ==
						    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron(0));
					}

					network::placed_atomic::Projection::Connection hardware_connection{
					    index_pre,
					    neuron_translation.at(projection.population_post)
					        .at(local_connection.index_post.first)
					        .at(local_connection.index_post.second)
					        .at(neuron_on_compartment),
					    network::placed_atomic::Projection::Connection::Weight(std::min<size_t>(
					        local_connection.weight.value() -
					            (p *
					             static_cast<size_t>(
					                 network::placed_atomic::Projection::Connection::Weight::max)),
					        network::placed_atomic::Projection::Connection::Weight::max))};
					indices.push_back(i);
					hardware_projection.connections.push_back(hardware_connection);
				}
			}
		}
		auto const hardware_descriptor = builder.add(hardware_projection);
		for (size_t i = 0; i < indices.size(); ++i) {
			projection_translation.insert(std::make_pair(
			    std::make_pair(descriptor, indices.at(i)), std::make_pair(hardware_descriptor, i)));
		}
		projection_descriptor_translation[descriptor] = hardware_descriptor;
	}

	// add plasticity rules
	NetworkGraph::PlasticityRuleTranslation plasticity_rule_translation;
	for (auto const& [d, plasticity_rule] : network->plasticity_rules) {
		network::placed_atomic::PlasticityRule hardware_plasticity_rule;
		for (auto const& d : plasticity_rule.projections) {
			hardware_plasticity_rule.projections.push_back(projection_descriptor_translation.at(d));
		}
		for (auto const& d : plasticity_rule.populations) {
			network::placed_atomic::PlasticityRule::PopulationHandle hardware_handle;
			hardware_handle.descriptor = population_translation.at(d.descriptor);
			for (auto const& neuron : d.neuron_readout_sources) {
				for (auto const& [_, denmems] : neuron) {
					hardware_handle.neuron_readout_sources.insert(
					    hardware_handle.neuron_readout_sources.end(), denmems.begin(),
					    denmems.end());
				}
			}
			hardware_plasticity_rule.populations.push_back(hardware_handle);
		}
		hardware_plasticity_rule.kernel = plasticity_rule.kernel;
		hardware_plasticity_rule.timer = plasticity_rule.timer;
		hardware_plasticity_rule.enable_requires_one_source_per_row_in_order =
		    plasticity_rule.enable_requires_one_source_per_row_in_order;
		hardware_plasticity_rule.recording = plasticity_rule.recording;
		plasticity_rule_translation[d] = builder.add(hardware_plasticity_rule);
	}

	result.m_network = network;
	result.m_population_translation = population_translation;
	result.m_neuron_translation = neuron_translation;
	result.m_projection_translation = projection_translation;
	result.m_plasticity_rule_translation = plasticity_rule_translation;

	// build graph translation
	auto const synapse_vertices = result.get_synapse_vertices();
	for (auto const& [d, projection] : network->projections) {
		auto const& local_synapse_vertices = synapse_vertices.at(d);
		auto const& population_post =
		    std::get<Population>(network->populations.at(projection.population_post));
		result.m_graph_translation.projections[d].resize(projection.connections.size());
		for (size_t i = 0; i < projection.connections.size(); ++i) {
			auto const& local_connection = projection.connections.at(i);
			auto const ans = population_post.neurons.at(local_connection.index_post.first)
			                     .coordinate.get_placed_compartments()
			                     .at(local_connection.index_post.second);
			for (auto const& an_target : routing_result.connection_routing_result.at(d)
			                                 .at(i)
			                                 .atomic_neurons_on_target_compartment) {
				auto const an = ans.at(an_target);
				auto const vertex_descriptor =
				    local_synapse_vertices.at(an.toNeuronRowOnDLS().toHemisphereOnDLS());
				auto const& vertex_property = std::get<signal_flow::vertex::SynapseArrayViewSparse>(
				    result.get_graph().get_vertex_property(vertex_descriptor));
				size_t const index_on_vertex = std::distance(
				    vertex_property.get_columns().begin(),
				    std::find(
				        vertex_property.get_columns().begin(), vertex_property.get_columns().end(),
				        an.toNeuronColumnOnDLS()));
				result.m_graph_translation.projections[d].at(i).push_back(
				    std::pair{vertex_descriptor, index_on_vertex});
			}
		}
	}
	auto const neuron_vertices = result.get_neuron_vertices();
	for (auto const& [d, pop] : network->populations) {
		if (!std::holds_alternative<Population>(pop)) {
			continue;
		}
		auto const& population = std::get<Population>(pop);
		auto const& local_neuron_vertices = neuron_vertices.at(d);
		result.m_graph_translation.populations[d].resize(population.neurons.size());
		for (size_t neuron_on_population = 0; neuron_on_population < population.neurons.size();
		     ++neuron_on_population) {
			for (auto const& [compartment_on_neuron, atomic_neurons] :
			     population.neurons.at(neuron_on_population).coordinate.get_placed_compartments()) {
				for (size_t atomic_neuron_on_compartment = 0;
				     atomic_neuron_on_compartment < atomic_neurons.size();
				     ++atomic_neuron_on_compartment) {
					auto const& atomic_neuron = atomic_neurons.at(atomic_neuron_on_compartment);
					auto const& local_neuron_vertex = local_neuron_vertices.at(
					    atomic_neuron.toNeuronRowOnDLS().toHemisphereOnDLS());
					auto const& vertex_property = std::get<signal_flow::vertex::NeuronView>(
					    result.get_graph().get_vertex_property(local_neuron_vertex));
					auto const& columns = vertex_property.get_columns();
					auto const column_iterator = std::find(
					    columns.begin(), columns.end(), atomic_neuron.toNeuronColumnOnDLS());
					assert(column_iterator != columns.end());
					size_t const neuron_column_index =
					    std::distance(columns.begin(), column_iterator);
					result.m_graph_translation.populations.at(d)
					    .at(neuron_on_population)[compartment_on_neuron]
					    .emplace_back(std::pair{local_neuron_vertex, neuron_column_index});
				}
			}
		}
	}

	result.m_construction_duration = result.m_hardware_network_graph.m_construction_duration;
	result.m_verification_duration = result.m_hardware_network_graph.m_verification_duration;
	result.m_routing_duration = result.m_hardware_network_graph.m_routing_duration;
	return result;
}


void update_network_graph(NetworkGraph& network_graph, std::shared_ptr<Network> const& network)
{
	hate::Timer timer;
	log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("grenade.update_network_graph");

	if (requires_routing(network, network_graph.m_network)) {
		throw std::runtime_error(
		    "Network graph can only be updated if no new routing is required.");
	}

	auto const connection_routing_result = build_connection_routing(network);
	placed_atomic::update_network_graph(
	    network_graph.m_hardware_network_graph,
	    build_atomic_network(network, connection_routing_result));
	network_graph.m_network = network;

	LOG4CXX_TRACE(
	    logger, "Updated hardware graph representation of network in " << timer.print() << ".");
}

} // namespace grenade::vx::network::placed_logical
