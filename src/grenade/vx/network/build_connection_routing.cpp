#include "grenade/vx/network/build_connection_routing.h"

#include "hate/math.h"
#include <stdexcept>

namespace grenade::vx::network {

ConnectionRoutingResult build_connection_routing(std::shared_ptr<Network> const& network)
{
	if (!network) {
		throw std::runtime_error("Building connection routing requires network, but supplied "
		                         "pointer to network is null.");
	}

	ConnectionRoutingResult result;

	// distribute weight onto multiple synapses
	// we split the weight like: 123 -> 63 + 60
	// we distribute over all neurons in every compartment
	halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> in_degree;
	in_degree.fill(0);
	for (auto const& [descriptor, projection] : network->projections) {
		auto& local_result = result[descriptor];
		local_result.resize(projection.connections.size());

		std::vector<size_t> num_synapses;
		for (auto const& connection : projection.connections) {
			num_synapses.push_back(std::max(
			    hate::math::round_up_integer_division(
			        connection.weight.value(), lola::vx::v3::SynapseMatrix::Weight::max),
			    static_cast<size_t>(1)));
		}
		if (num_synapses.empty()) {
			continue;
		}
		auto const max_num_synapses = *std::max_element(num_synapses.begin(), num_synapses.end());
		for (size_t p = 0; p < max_num_synapses; ++p) {
			auto const& population_post =
			    std::get<Population>(network->populations.at(projection.population_post));
			auto const& population_pre = network->populations.at(projection.population_pre);
			std::vector<size_t> indices;
			for (size_t i = 0; i < projection.connections.size(); ++i) {
				// only find new hardware synapse, if the current connection requires another one
				if (num_synapses.at(i) <= p) {
					continue;
				}
				auto const& local_connection = projection.connections.at(i);
				// find neurons with matching receptor
				auto const& receptors =
				    population_post.neurons.at(local_connection.index_post.first)
				        .compartments.at(local_connection.index_post.second)
				        .receptors;
				auto const get_neuron_post = [&](size_t index) {
					return population_post.neurons.at(local_connection.index_post.first)
					    .coordinate.get_placed_compartments()
					    .at(local_connection.index_post.second)
					    .at(index);
				};
				std::set<size_t> neurons_with_matching_receptor;
				for (size_t i = 0; i < receptors.size(); ++i) {
					if (receptors.at(i).contains(projection.receptor)) {
						if ((std::holds_alternative<BackgroundSourcePopulation>(population_pre) &&
						     std::get<BackgroundSourcePopulation>(population_pre)
						         .coordinate.contains(
						             get_neuron_post(i).toNeuronRowOnDLS().toHemisphereOnDLS())) ||
						    !std::holds_alternative<BackgroundSourcePopulation>(population_pre)) {
							neurons_with_matching_receptor.insert(i);
						}
					}
				}
				if (neurons_with_matching_receptor.empty()) {
					throw std::runtime_error(
					    "No neuron on compartment features receptor requested by projection.");
				}
				// choose neuron with matching receptor and smallest current in_degree
				auto const neuron_on_compartment = *std::min_element(
				    neurons_with_matching_receptor.begin(), neurons_with_matching_receptor.end(),
				    [in_degree, get_neuron_post](auto const& a, auto const& b) {
					    return in_degree[get_neuron_post(a)] < in_degree[get_neuron_post(b)];
				    });
				// choice performed, in_degree++ of chosen neuron
				in_degree[get_neuron_post(neuron_on_compartment)]++;

				local_result.at(i).atomic_neurons_on_target_compartment.push_back(
				    neuron_on_compartment);
				indices.push_back(i);
			}
		}
	}
	return result;
}

} // namespace grenade::vx::network
