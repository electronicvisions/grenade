#include "grenade/vx/network/placed_logical/network_graph_builder.h"

#include "grenade/vx/network/placed_atomic/network_builder.h"
#include "grenade/vx/network/placed_logical/build_connection_routing.h"
#include "hate/math.h"
#include "hate/variant.h"
#include <map>

namespace grenade::vx::network::placed_logical {

NetworkGraph build_network_graph(std::shared_ptr<Network> const& network)
{
	assert(network);

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
		        [&builder](auto const& pop /* BackgroundSpikeSoucePopulation, ExternalPopulation stay the same */) { return builder.add(pop); }},
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
	auto const connection_routing = build_connection_routing(network);
	NetworkGraph::ProjectionTranslation projection_translation;
	std::map<ProjectionDescriptor, std::vector<network::placed_atomic::ProjectionDescriptor>>
	    projection_descriptor_translation;
	for (auto const& [descriptor, projection] : network->projections) {
		auto const max_num_synapses =
		    std::max_element(
		        connection_routing.at(descriptor).begin(), connection_routing.at(descriptor).end(),
		        [](auto const& a, auto const& b) {
			        return a.atomic_neurons_on_target_compartment.size() <
			               b.atomic_neurons_on_target_compartment.size();
		        })
		        ->atomic_neurons_on_target_compartment.size();
		for (size_t p = 0; p < max_num_synapses; ++p) {
			network::placed_atomic::Projection hardware_projection;
			hardware_projection.receptor_type = projection.receptor.type;
			hardware_projection.population_pre =
			    population_translation.at(projection.population_pre);
			hardware_projection.population_post =
			    population_translation.at(projection.population_post);
			auto const& population_pre = network->populations.at(projection.population_pre);
			std::vector<size_t> indices;
			for (size_t i = 0; i < projection.connections.size(); ++i) {
				// only find new hardware synapse, if the current connection requires another one
				if (connection_routing.at(descriptor)
				        .at(i)
				        .atomic_neurons_on_target_compartment.size() <= p) {
					continue;
				}
				auto const& local_connection = projection.connections.at(i);
				auto const neuron_on_compartment = connection_routing.at(descriptor)
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
				            (p * static_cast<size_t>(
				                     network::placed_atomic::Projection::Connection::Weight::max)),
				        network::placed_atomic::Projection::Connection::Weight::max))};
				indices.push_back(i);
				hardware_projection.connections.push_back(hardware_connection);
			}
			auto const hardware_descriptor = builder.add(hardware_projection);
			for (size_t i = 0; i < indices.size(); ++i) {
				projection_translation.insert(std::make_pair(
				    std::make_pair(descriptor, indices.at(i)),
				    std::make_pair(hardware_descriptor, i)));
			}
			projection_descriptor_translation[descriptor].push_back(hardware_descriptor);
		}
	}

	// add plasticity rules
	NetworkGraph::PlasticityRuleTranslation plasticity_rule_translation;
	for (auto const& [d, plasticity_rule] : network->plasticity_rules) {
		network::placed_atomic::PlasticityRule hardware_plasticity_rule;
		for (auto const& d : plasticity_rule.projections) {
			hardware_plasticity_rule.projections.insert(
			    hardware_plasticity_rule.projections.end(),
			    projection_descriptor_translation.at(d).begin(),
			    projection_descriptor_translation.at(d).end());
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

	NetworkGraph result;
	result.m_network = network;
	result.m_hardware_network = builder.done();
	result.m_population_translation = population_translation;
	result.m_neuron_translation = neuron_translation;
	result.m_projection_translation = projection_translation;
	result.m_plasticity_rule_translation = plasticity_rule_translation;
	return result;
}

} // namespace grenade::vx::network::placed_logical
