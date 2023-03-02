#include "grenade/vx/network/placed_atomic/generate_input.h"

#include <algorithm>

namespace grenade::vx::network::placed_atomic {

InputGenerator::InputGenerator(NetworkGraph const& network_graph, size_t batch_size) :
    m_data(), m_network_graph(network_graph)
{
	if (!m_network_graph.get_event_input_vertex()) {
		throw std::runtime_error("Network graph does not feature an event input vertex.");
	}
	std::vector<signal_flow::TimedSpikeToChipSequence> spike_batch(batch_size);
	m_data.data.insert({*m_network_graph.get_event_input_vertex(), spike_batch});
}

void InputGenerator::add(
    std::vector<common::Time> const& times, PopulationDescriptor const population)
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

	auto const& neurons = m_network_graph.get_spike_labels().at(population);

	signal_flow::TimedSpikeToChipSequence batch_entry;
	batch_entry.reserve(times.size() * neurons.size());
	for (auto const time : times) {
		for (auto const& neuron : neurons) {
			for (auto const& label : neuron) {
				assert(label);
				batch_entry.push_back(grenade::vx::signal_flow::TimedSpikeToChip{
				    time, haldls::vx::v3::SpikePack1ToChip(
				              haldls::vx::v3::SpikePack1ToChip::labels_type{*label})});
			}
		}
	}
	auto& spike_batch = std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
	    m_data.data.at(*m_network_graph.get_event_input_vertex()));
	for (size_t b = 0; b < spike_batch.size(); ++b) {
		spike_batch.at(b).insert(spike_batch.at(b).end(), batch_entry.begin(), batch_entry.end());
	}
}

void InputGenerator::add(
    std::vector<std::vector<common::Time>> const& times, PopulationDescriptor const population)
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

	auto const& neurons = m_network_graph.get_spike_labels().at(population);

	if (times.size() != neurons.size()) {
		throw std::runtime_error(
		    "Times number of neurons does not match expected number from spike labels.");
	}

	size_t size = 0;
	for (auto const& ts : times) {
		size += ts.size();
	}

	signal_flow::TimedSpikeToChipSequence batch_entry;
	batch_entry.reserve(size);
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
				assert(label);
				batch_entry.push_back(grenade::vx::signal_flow::TimedSpikeToChip{
				    time, haldls::vx::v3::SpikePack1ToChip(
				              haldls::vx::v3::SpikePack1ToChip::labels_type{*label})});
			}
		}
	}
	auto& spike_batch = std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
	    m_data.data.at(*m_network_graph.get_event_input_vertex()));
	for (size_t b = 0; b < spike_batch.size(); ++b) {
		spike_batch.at(b).insert(spike_batch.at(b).end(), batch_entry.begin(), batch_entry.end());
	}
}

void InputGenerator::add(
    std::vector<std::vector<std::vector<common::Time>>> const& times,
    PopulationDescriptor const population)
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

	auto const& neurons = m_network_graph.get_spike_labels().at(population);

	assert(
	    times.size() == std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
	                        m_data.data.at(*m_network_graph.get_event_input_vertex()))
	                        .size());
	for (size_t b = 0; b < times.size(); ++b) {
		auto const& batch_times = times.at(b);

		if (batch_times.size() != neurons.size()) {
			throw std::runtime_error(
			    "Times number of neurons does not match expected number from spike labels.");
		}

		auto& data_spikes = std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
		                        m_data.data.at(*m_network_graph.get_event_input_vertex()))
		                        .at(b);

		size_t size = 0;
		for (auto const& ts : batch_times) {
			size += ts.size();
		}

		data_spikes.reserve(size);
		for (size_t i = 0; i < batch_times.size(); ++i) {
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
			for (auto const time : batch_times.at(i)) {
				for (auto const label : labels) {
					assert(label);
					data_spikes.push_back(grenade::vx::signal_flow::TimedSpikeToChip{
					    time, haldls::vx::v3::SpikePack1ToChip(
					              haldls::vx::v3::SpikePack1ToChip::labels_type{*label})});
				}
			}
		}
	}
}

signal_flow::IODataMap InputGenerator::done()
{
	assert(m_network_graph.get_event_input_vertex());
	auto& spikes = std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
	    m_data.data.at(*m_network_graph.get_event_input_vertex()));
	for (auto& batch : spikes) {
		std::stable_sort(batch.begin(), batch.end(), [](auto const& a, auto const& b) {
			return a.time < b.time;
		});
	}
	signal_flow::IODataMap ret;
	std::vector<signal_flow::TimedSpikeToChipSequence> spike_batch(m_data.batch_size());
	assert(m_network_graph.get_event_input_vertex());
	ret.data.insert({*m_network_graph.get_event_input_vertex(), spike_batch});
	std::swap(ret, m_data);
	return ret;
}

} // namespace grenade::vx::network::placed_atomic
