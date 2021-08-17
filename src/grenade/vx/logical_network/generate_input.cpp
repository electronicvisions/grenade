#include "grenade/vx/logical_network/generate_input.h"

#include <algorithm>

namespace grenade::vx::logical_network {

InputGenerator::InputGenerator(
    NetworkGraph const& network_graph, network::NetworkGraph const& hardware_network_graph) :
    m_data(), m_network_graph(network_graph), m_hardware_network_graph(hardware_network_graph)
{
	if (m_network_graph.get_hardware_network() != m_hardware_network_graph.get_network()) {
		throw std::runtime_error(
		    "Input generator requires logical and hardware network graph with matching network.");
	}
	if (!m_hardware_network_graph.get_event_input_vertex()) {
		throw std::runtime_error("Network graph does not feature an event input vertex.");
	}
	m_data.data.insert(
	    {*m_hardware_network_graph.get_event_input_vertex(), std::vector<TimedSpikeSequence>{{}}});
}

void InputGenerator::add(
    std::vector<TimedSpike::Time> const& times, PopulationDescriptor const population)
{
	// Exit early if population is not connected to any other population
	auto const has_population = [population](auto const& projection) {
		return (projection.second.population_pre == population);
	};
	if (std::none_of(
	        m_network_graph.get_network()->projections.begin(),
	        m_network_graph.get_network()->projections.end(), has_population)) {
		return;
	}

	auto& data_spikes = std::get<std::vector<TimedSpikeSequence>>(
	                        m_data.data.at(*m_hardware_network_graph.get_event_input_vertex()))
	                        .at(0);

	auto const& neurons = m_hardware_network_graph.get_spike_labels().at(
	    m_network_graph.get_population_translation().at(population));

	data_spikes.reserve(times.size() * neurons.size());
	for (auto const time : times) {
		for (auto const& neuron : neurons) {
			for (auto const label : neuron) {
				if (label) {
					data_spikes.push_back(grenade::vx::TimedSpike{
					    time, haldls::vx::v3::SpikePack1ToChip(
					              haldls::vx::v3::SpikePack1ToChip::labels_type{*label})});
				}
			}
		}
	}
}

void InputGenerator::add(
    std::vector<std::vector<TimedSpike::Time>> const& times, PopulationDescriptor const population)
{
	// Exit early if population is not connected to any other population
	auto const has_population = [population](auto const& projection) {
		return (projection.second.population_pre == population);
	};
	if (std::none_of(
	        m_network_graph.get_network()->projections.begin(),
	        m_network_graph.get_network()->projections.end(), has_population)) {
		return;
	}

	auto& data_spikes = std::get<std::vector<TimedSpikeSequence>>(
	                        m_data.data.at(*m_hardware_network_graph.get_event_input_vertex()))
	                        .at(0);

	auto const& neurons = m_hardware_network_graph.get_spike_labels().at(
	    m_network_graph.get_population_translation().at(population));

	if (times.size() != neurons.size()) {
		throw std::runtime_error(
		    "Times number of neurons does not match expected number from spike labels.");
	}

	size_t size = 0;
	for (auto const& ts : times) {
		size += ts.size();
	}

	data_spikes.reserve(size);
	for (size_t i = 0; i < times.size(); ++i) {
		// Exit early if neuron is not connected to any other neuron
		auto const has_neuron = [population, i](auto const& projection) {
			if (projection.second.population_pre != population) {
				return false;
			}

			// Check if neuron is part of any connection
			for (auto const& c : projection.second.connections) {
				if (c.index_pre == i) {
					return true;
				}
			}
			return false;
		};

		if (std::none_of(
		        m_network_graph.get_network()->projections.begin(),
		        m_network_graph.get_network()->projections.end(), has_neuron)) {
			continue;
		}

		auto const labels = neurons.at(i);
		for (auto const time : times.at(i)) {
			for (auto const label : labels) {
				if (label) {
					data_spikes.push_back(grenade::vx::TimedSpike{
					    time, haldls::vx::v3::SpikePack1ToChip(
					              haldls::vx::v3::SpikePack1ToChip::labels_type{*label})});
				}
			}
		}
	}
}

IODataMap InputGenerator::done()
{
	assert(m_hardware_network_graph.get_event_input_vertex());
	auto& spikes = std::get<std::vector<TimedSpikeSequence>>(
	                   m_data.data.at(*m_hardware_network_graph.get_event_input_vertex()))
	                   .at(0);
	std::stable_sort(
	    spikes.begin(), spikes.end(), [](auto const& a, auto const& b) { return a.time < b.time; });
	return std::move(m_data);
}

} // namespace grenade::vx::logical_network
