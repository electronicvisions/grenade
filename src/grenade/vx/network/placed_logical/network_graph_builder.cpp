#include "grenade/vx/network/placed_logical/network_graph_builder.h"

#include "grenade/vx/network/placed_atomic/network_builder.h"
#include "grenade/vx/network/placed_logical/build_atomic_network.h"
#include "grenade/vx/network/placed_logical/build_connection_routing.h"
#include "grenade/vx/network/placed_logical/build_connection_weight_split.h"
#include "grenade/vx/network/placed_logical/requires_routing.h"
#include "hate/math.h"
#include "hate/timer.h"
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
	result.m_connection_routing_result = routing_result.connection_routing_result;

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
	result.m_construction_duration = result.m_hardware_network_graph.m_construction_duration;
	result.m_verification_duration = result.m_hardware_network_graph.m_verification_duration;
	result.m_routing_duration = result.m_hardware_network_graph.m_routing_duration;

	// build graph translation
	auto const synapse_vertices = result.get_synapse_vertices();
	for (auto const& [d, projection] : network->projections) {
		auto const& local_synapse_vertices = synapse_vertices.at(d);
		auto const& population_post =
		    std::get<Population>(network->populations.at(projection.population_post));
		result.m_graph_translation.projections[d].resize(projection.connections.size());
		std::map<halco::hicann_dls::vx::v3::HemisphereOnDLS, size_t> index_per_hemisphere;
		for (size_t i = 0; i < projection.connections.size(); ++i) {
			auto const& local_connection = projection.connections.at(i);
			auto const ans = population_post.neurons.at(local_connection.index_post.first)
			                     .coordinate.get_placed_compartments()
			                     .at(local_connection.index_post.second);
			for (auto const& an_target : routing_result.connection_routing_result.at(d)
			                                 .at(i)
			                                 .atomic_neurons_on_target_compartment) {
				auto const an = ans.at(an_target);
				auto const hemisphere = an.toNeuronRowOnDLS().toHemisphereOnDLS();
				if (!index_per_hemisphere.contains(hemisphere)) {
					index_per_hemisphere[hemisphere] = 0;
				}
				auto const vertex_descriptor =
				    local_synapse_vertices.at(an.toNeuronRowOnDLS().toHemisphereOnDLS());
				result.m_graph_translation.projections[d].at(i).push_back(
				    std::pair{vertex_descriptor, index_per_hemisphere.at(hemisphere)});
				index_per_hemisphere.at(hemisphere)++;
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

	return result;
}


using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

void update_network_graph(NetworkGraph& network_graph, std::shared_ptr<Network> const& network)
{
	hate::Timer timer;
	log4cxx::LoggerPtr logger =
	    log4cxx::Logger::getLogger("grenade.placed_logical.update_network_graph");

	if (requires_routing(network, network_graph)) {
		throw std::runtime_error(
		    "Network graph can only be updated if no new routing is required.");
	}

	// get whether projection requires weight changes
	auto const requires_change = [](Projection const& projection,
	                                Projection const& old_projection) {
		for (size_t i = 0; i < projection.connections.size(); ++i) {
			auto const& connection = projection.connections.at(i);
			auto const& old_connection = old_projection.connections.at(i);
			if (connection.weight != old_connection.weight) {
				return true;
			}
		}
		return false;
	};

	// update synapse array view in hardware graph corresponding to projection
	auto const update_weights = [](NetworkGraph& network_graph,
	                               std::shared_ptr<Network> const& network,
	                               ProjectionDescriptor const descriptor) {
		std::map<
		    signal_flow::Graph::vertex_descriptor,
		    signal_flow::vertex::SynapseArrayViewSparse::Synapses>
		    synapses;
		auto const synapse_vertices = network_graph.get_synapse_vertices();
		for (auto const [_, vertex_descriptor] : synapse_vertices.at(descriptor)) {
			auto const& old_synapse_array_view =
			    std::get<signal_flow::vertex::SynapseArrayViewSparse>(
			        network_graph.get_graph().get_vertex_property(vertex_descriptor));
			auto const old_synapses = old_synapse_array_view.get_synapses();
			synapses[vertex_descriptor] = signal_flow::vertex::SynapseArrayViewSparse::Synapses{
			    old_synapses.begin(), old_synapses.end()};
		}
		auto const& old_projection = network_graph.get_network()->projections.at(descriptor);
		auto const& projection = network->projections.at(descriptor);
		auto const& translation = network_graph.get_graph_translation().projections;
		for (size_t i = 0; i < old_projection.connections.size(); ++i) {
			auto const& connection = projection.connections.at(i);
			auto const& old_connection = old_projection.connections.at(i);
			if (connection.weight != old_connection.weight) {
				auto const& synapse_indices = translation.at(descriptor).at(i);
				auto const weight_split =
				    build_connection_weight_split(connection.weight, synapse_indices.size());
				for (size_t s = 0; s < synapse_indices.size(); ++s) {
					synapses.at(synapse_indices.at(s).first)
					    .at(synapse_indices.at(s).second)
					    .weight = weight_split.at(s);
				}
			}
		}
		for (auto const [_, vertex_descriptor] : synapse_vertices.at(descriptor)) {
			auto const& old_synapse_array_view =
			    std::get<signal_flow::vertex::SynapseArrayViewSparse>(
			        network_graph.get_graph().get_vertex_property(vertex_descriptor));
			auto const old_rows = old_synapse_array_view.get_rows();
			auto rows =
			    signal_flow::vertex::SynapseArrayViewSparse::Rows{old_rows.begin(), old_rows.end()};
			auto const old_columns = old_synapse_array_view.get_columns();
			auto columns = signal_flow::vertex::SynapseArrayViewSparse::Columns{
			    old_columns.begin(), old_columns.end()};
			signal_flow::vertex::SynapseArrayViewSparse synapse_array_view(
			    old_synapse_array_view.get_synram(), std::move(rows), std::move(columns),
			    std::move(synapses.at(vertex_descriptor)));
			network_graph.m_hardware_network_graph.m_graph.update(
			    vertex_descriptor, std::move(synapse_array_view));
		}
	};

	// update MADC in graph such that it resembles the configuration from network to update towards
	auto const update_madc = [](NetworkGraph& network_graph, Network const& network) {
		if (!network.madc_recording || !network_graph.m_network->madc_recording) {
			throw std::runtime_error("Updating network graph only possible, if MADC recording is "
			                         "neither added nor removed.");
		}
		auto const& population =
		    std::get<Population>(network.populations.at(network.madc_recording->population));
		auto const neuron = population.neurons.at(network.madc_recording->neuron_on_population)
		                        .coordinate.get_placed_compartments()
		                        .at(network.madc_recording->compartment_on_neuron)
		                        .at(network.madc_recording->atomic_neuron_on_compartment);
		signal_flow::vertex::MADCReadoutView madc_readout(neuron, network.madc_recording->source);
		auto const neuron_vertex_descriptor =
		    network_graph.get_neuron_vertices()
		        .at(network.madc_recording->population)
		        .at(neuron.toNeuronRowOnDLS().toHemisphereOnDLS());
		auto const& neuron_vertex = std::get<signal_flow::vertex::NeuronView>(
		    network_graph.get_graph().get_vertex_property(neuron_vertex_descriptor));
		auto const& columns = neuron_vertex.get_columns();
		auto const in_view_location = static_cast<size_t>(std::distance(
		    columns.begin(),
		    std::find(columns.begin(), columns.end(), neuron.toNeuronColumnOnDLS())));
		assert(in_view_location < columns.size());
		std::vector<signal_flow::Input> inputs{
		    {neuron_vertex_descriptor, {in_view_location, in_view_location}}};
		assert(network_graph.get_madc_sample_output_vertex());
		assert(
		    boost::in_degree(
		        *(network_graph.get_madc_sample_output_vertex()),
		        network_graph.get_graph().get_graph()) == 1);
		auto const edges = boost::in_edges(
		    *(network_graph.get_madc_sample_output_vertex()),
		    network_graph.get_graph().get_graph());
		auto const madc_vertex =
		    boost::source(*(edges.first), network_graph.get_graph().get_graph());
		network_graph.m_hardware_network_graph.m_graph.update_and_relocate(
		    madc_vertex, std::move(madc_readout), inputs);
	};

	// update plasticity rule in graph such that it resembles the configuration from network to
	// update towards
	auto const update_plasticity_rule = [](NetworkGraph& network_graph,
	                                       PlasticityRule const& new_rule,
	                                       PlasticityRuleDescriptor descriptor) {
		if (!network_graph.m_network->plasticity_rules.contains(descriptor)) {
			throw std::runtime_error("Updating network graph only possible, if plasticity rule is "
			                         "neither added nor removed.");
		}
		if (network_graph.m_network->plasticity_rules.at(descriptor).recording !=
		    new_rule.recording) {
			throw std::runtime_error("Updating network graph only possible, if plasticity rule "
			                         "recording is not changed.");
		}
		// TODO (Issue #3991): merge impl. from here and add_plasticity_rule below, requires rework
		// of resources in order to align interface to NetworkGraph.
		auto const vertex_descriptor = network_graph.get_plasticity_rule_vertices().at(descriptor);
		std::vector<signal_flow::Input> inputs;
		auto const synapse_vertices = network_graph.get_synapse_vertices();
		for (auto const& d : new_rule.projections) {
			for (auto const& [_, p] : synapse_vertices.at(d)) {
				inputs.push_back({p});
			}
		}
		auto const neuron_vertices = network_graph.get_neuron_vertices();
		for (auto const& d : new_rule.populations) {
			for (auto const& [_, p] : neuron_vertices.at(d.descriptor)) {
				inputs.push_back({p});
			}
		}
		auto const& old_rule = std::get<signal_flow::vertex::PlasticityRule>(
		    network_graph.get_graph().get_vertex_property(vertex_descriptor));
		signal_flow::vertex::PlasticityRule vertex(
		    new_rule.kernel,
		    signal_flow::vertex::PlasticityRule::Timer{
		        signal_flow::vertex::PlasticityRule::Timer::Value(new_rule.timer.start.value()),
		        signal_flow::vertex::PlasticityRule::Timer::Value(new_rule.timer.period.value()),
		        new_rule.timer.num_periods},
		    old_rule.get_synapse_view_shapes(), old_rule.get_neuron_view_shapes(),
		    new_rule.recording);
		network_graph.m_hardware_network_graph.m_graph.update_and_relocate(
		    vertex_descriptor, std::move(vertex), inputs);
	};

	assert(network);
	assert(network_graph.m_network);

	for (auto const& [descriptor, projection] : network->projections) {
		auto const& old_projection = network_graph.m_network->projections.at(descriptor);
		if (!requires_change(projection, old_projection)) {
			continue;
		}
		update_weights(network_graph, network, descriptor);
	}
	if (network_graph.m_network->madc_recording != network->madc_recording) {
		update_madc(network_graph, *network);
	}
	if (network_graph.m_network->plasticity_rules != network->plasticity_rules) {
		for (auto const& [descriptor, new_rule] : network->plasticity_rules) {
			update_plasticity_rule(network_graph, new_rule, descriptor);
		}
	}
	network_graph.m_network = network;
	network_graph.m_hardware_network_graph.m_network =
	    build_atomic_network(network, network_graph.get_connection_routing_result());

	LOG4CXX_TRACE(
	    logger, "Updated hardware graph representation of network in " << timer.print() << ".");
	//	network_graph.m_hardware_network_graph.m_construction_duration +=
	//	    std::chrono::microseconds(timer.get_us());
}

} // namespace grenade::vx::network::placed_logical
