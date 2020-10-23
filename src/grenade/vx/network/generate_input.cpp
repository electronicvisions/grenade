#include "grenade/vx/network/generate_input.h"

#include <algorithm>

namespace grenade::vx::network {

InputGenerator::InputGenerator(NetworkGraph const& network_graph) :
    m_data(), m_network_graph(network_graph)
{
	if (!m_network_graph.event_input_vertex) {
		throw std::runtime_error("Network graph does not feature an event input vertex.");
	}
	m_data.spike_events.insert({*m_network_graph.event_input_vertex, {{}}});
}

void InputGenerator::add(
    std::vector<TimedSpike::Time> const& times, PopulationDescriptor const population)
{
	auto& data_spikes = m_data.spike_events.at(*m_network_graph.event_input_vertex).at(0);

	auto const& spike_labels = m_network_graph.spike_labels.at(population);

	data_spikes.reserve(times.size() * spike_labels.size());
	for (auto const time : times) {
		for (auto const label : spike_labels) {
			data_spikes.push_back(grenade::vx::TimedSpike{
			    time, haldls::vx::v2::SpikePack1ToChip(
			              haldls::vx::v2::SpikePack1ToChip::labels_type{label})});
		}
	}
}

void InputGenerator::add(
    std::vector<std::vector<TimedSpike::Time>> const& times, PopulationDescriptor const population)
{
	auto& data_spikes = m_data.spike_events.at(*m_network_graph.event_input_vertex).at(0);

	auto const& spike_labels = m_network_graph.spike_labels.at(population);

	if (times.size() != spike_labels.size()) {
		throw std::runtime_error(
		    "Times number of neurons does not match expected number from spike labels.");
	}

	size_t size = 0;
	for (auto const& ts : times) {
		size += ts.size();
	}

	data_spikes.reserve(size);
	for (size_t i = 0; i < times.size(); ++i) {
		auto const label = spike_labels.at(i);
		for (auto const time : times.at(i)) {
			data_spikes.push_back(grenade::vx::TimedSpike{
			    time, haldls::vx::v2::SpikePack1ToChip(
			              haldls::vx::v2::SpikePack1ToChip::labels_type{label})});
		}
	}
}

IODataMap InputGenerator::done()
{
	assert(m_network_graph.event_input_vertex);
	auto& spikes = m_data.spike_events.at(*m_network_graph.event_input_vertex).at(0);
	std::stable_sort(
	    spikes.begin(), spikes.end(), [](auto const& a, auto const& b) { return a.time < b.time; });
	return std::move(m_data);
}

} // namespace grenade::vx::network
