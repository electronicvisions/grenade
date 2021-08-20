#include "grenade/vx/logical_network/network_graph_builder.h"

#include "grenade/vx/network/network_builder.h"
#include "hate/math.h"
#include "hate/variant.h"
#include <map>

namespace grenade::vx::logical_network {

NetworkGraph build_network_graph(std::shared_ptr<Network> const& network)
{
	assert(network);

	// construct hardware network
	network::NetworkBuilder builder;

	// add populations, currently direct translation
	NetworkGraph::PopulationTranslation population_translation;
	for (auto const& [descriptor, population] : network->populations) {
		population_translation[descriptor] = std::visit(
		    hate::overloaded{
		        [&builder](Population const& pop) {
			        network::Population hardware_pop(pop.neurons, pop.enable_record_spikes);
			        return builder.add(hardware_pop);
		        },
		        [&builder](auto const& pop /* BackgroundSpikeSoucePopulation, ExternalPopulation stay the same */) { return builder.add(pop); }},
		    population);
	}

	// add MADC recording if present
	if (network->madc_recording) {
		network::MADCRecording hardware_madc_recording;
		hardware_madc_recording.population =
		    population_translation.at(network->madc_recording->population);
		hardware_madc_recording.index = network->madc_recording->index;
		hardware_madc_recording.source = network->madc_recording->source;
		builder.add(hardware_madc_recording);
	}

	// add projections, distribute weight onto multiple synapses
	// we split the weight like: 123 -> 63 + 60
	NetworkGraph::ProjectionTranslation projection_translation;
	std::map<ProjectionDescriptor, std::vector<network::ProjectionDescriptor>>
	    projection_descriptor_translation;
	for (auto const& [descriptor, projection] : network->projections) {
		std::vector<size_t> num_synapses;
		for (auto const& connection : projection.connections) {
			num_synapses.push_back(std::max(
			    hate::math::round_up_integer_division(
			        connection.weight.value(), network::Projection::Connection::Weight::max),
			    static_cast<size_t>(1)));
		}
		assert(!num_synapses.empty());
		auto const max_num_synapses = *std::max_element(num_synapses.begin(), num_synapses.end());
		for (size_t p = 0; p < max_num_synapses; ++p) {
			network::Projection hardware_projection;
			hardware_projection.receptor_type = projection.receptor.type;
			hardware_projection.population_pre =
			    population_translation.at(projection.population_pre);
			hardware_projection.population_post =
			    population_translation.at(projection.population_post);
			auto const& population_post =
			    std::get<Population>(network->populations.at(projection.population_post));
			std::vector<size_t> indices;
			for (size_t i = 0; i < projection.connections.size(); ++i) {
				auto const& local_connection = projection.connections.at(i);
				auto const& receptors = population_post.receptors.at(local_connection.index_post);
				if (!receptors.contains(projection.receptor)) {
					throw std::runtime_error(
					    "Neuron does not feature receptor requested by projection.");
				}
				if (num_synapses.at(i) > p) {
					network::Projection::Connection hardware_connection{
					    local_connection.index_pre, local_connection.index_post,
					    network::Projection::Connection::Weight(std::min<size_t>(
					        local_connection.weight.value() -
					            (p *
					             static_cast<size_t>(network::Projection::Connection::Weight::max)),
					        network::Projection::Connection::Weight::max))};
					indices.push_back(i);
					hardware_projection.connections.push_back(hardware_connection);
				}
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
	for (auto const& [d, plasticity_rule] : network->plasticity_rules) {
		network::PlasticityRule hardware_plasticity_rule;
		for (auto const& d : plasticity_rule.projections) {
			hardware_plasticity_rule.projections.insert(
			    hardware_plasticity_rule.projections.end(),
			    projection_descriptor_translation.at(d).begin(),
			    projection_descriptor_translation.at(d).end());
		}
		hardware_plasticity_rule.kernel = plasticity_rule.kernel;
		hardware_plasticity_rule.timer = plasticity_rule.timer;
		hardware_plasticity_rule.enable_requires_one_source_per_row_in_order =
		    plasticity_rule.enable_requires_one_source_per_row_in_order;
		builder.add(hardware_plasticity_rule);
	}

	NetworkGraph result;
	result.m_network = network;
	result.m_hardware_network = builder.done();
	result.m_population_translation = population_translation;
	result.m_projection_translation = projection_translation;
	return result;
}

} // namespace grenade::vx::logical_network
