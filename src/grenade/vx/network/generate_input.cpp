#include "grenade/vx/network/generate_input.h"

#include <algorithm>

namespace grenade::vx::network {

InputGenerator::InputGenerator(NetworkGraph const& network_graph, size_t const batch_size) :
    m_data(), m_network_graph(network_graph)
{
	for (auto const& [id, execution_instance] :
	     m_network_graph.get_graph_translation().execution_instances) {
		if (!execution_instance.event_input_vertex) {
			continue;
		}
		std::vector<signal_flow::TimedSpikeToChipSequence> spike_batch(batch_size);
		m_data.data.insert({*execution_instance.event_input_vertex, spike_batch});
	}
}

namespace {

auto const neuron_is_used_by_projection = [](PopulationOnNetwork const& population,
                                             size_t i,
                                             auto const& projection) {
	if (projection.second.population_pre != population.toPopulationOnExecutionInstance()) {
		return false;
	}
	// Check if neuron is part of any connection
	for (auto const& c : projection.second.connections) {
		assert(c.index_pre.second == halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron());
		if (c.index_pre == std::pair{i, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron()}) {
			return true;
		}
	}
	return false;
};

auto const neuron_is_recorded = [](ExternalSourcePopulation const& pop, size_t i) {
	return pop.neurons.at(i).enable_record_spikes;
};

auto const neuron_is_used =
    [](NetworkGraph const& network_graph, PopulationOnNetwork const& population, size_t i) {
	    if (std::any_of(
	            network_graph.get_network()
	                ->execution_instances.at(population.toExecutionInstanceID())
	                .projections.begin(),
	            network_graph.get_network()
	                ->execution_instances.at(population.toExecutionInstanceID())
	                .projections.end(),
	            std::bind(neuron_is_used_by_projection, population, i, std::placeholders::_1))) {
		    return true;
	    }
	    auto const& pop = std::get<ExternalSourcePopulation>(
	        network_graph.get_network()
	            ->execution_instances.at(population.toExecutionInstanceID())
	            .populations.at(population.toPopulationOnExecutionInstance()));
	    return neuron_is_recorded(pop, i);
    };

} // namespace

void InputGenerator::add(
    std::vector<common::Time> const& times, PopulationOnNetwork const population)
{
	auto const neurons = m_network_graph.get_graph_translation()
	                         .execution_instances.at(population.toExecutionInstanceID())
	                         .spike_labels.at(population.toPopulationOnExecutionInstance());

	signal_flow::TimedSpikeToChipSequence batch_entry;
	batch_entry.reserve(times.size() * neurons.size());
	for (auto const time : times) {
		for (size_t i = 0; auto const& neuron : neurons) {
			if (!neuron_is_used(m_network_graph, population, i)) {
				continue;
			}
			assert(neuron.size() == 1);
			for (auto const& label :
			     neuron.at(halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron())) {
				assert(label);
				batch_entry.push_back(grenade::vx::signal_flow::TimedSpikeToChip{
				    time, haldls::vx::v3::SpikePack1ToChip(
				              haldls::vx::v3::SpikePack1ToChip::labels_type{*label})});
			}
			i++;
		}
	}
	auto& spike_batch = std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
	    m_data.data.at(*m_network_graph.get_graph_translation()
	                        .execution_instances.at(population.toExecutionInstanceID())
	                        .event_input_vertex));
	for (size_t b = 0; b < spike_batch.size(); ++b) {
		spike_batch.at(b).insert(spike_batch.at(b).end(), batch_entry.begin(), batch_entry.end());
	}
}

void InputGenerator::add(
    std::vector<std::vector<common::Time>> const& times, PopulationOnNetwork const population)
{
	auto const neurons = m_network_graph.get_graph_translation()
	                         .execution_instances.at(population.toExecutionInstanceID())
	                         .spike_labels.at(population.toPopulationOnExecutionInstance());

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
		if (!neuron_is_used(m_network_graph, population, i)) {
			continue;
		}
		auto const neuron = neurons.at(i);
		assert(neuron.size() == 1);
		auto const labels = neuron.at(halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron());
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
	    m_data.data.at(*m_network_graph.get_graph_translation()
	                        .execution_instances.at(population.toExecutionInstanceID())
	                        .event_input_vertex));
	for (size_t b = 0; b < spike_batch.size(); ++b) {
		spike_batch.at(b).insert(spike_batch.at(b).end(), batch_entry.begin(), batch_entry.end());
	}
}

void InputGenerator::add(
    std::vector<std::vector<std::vector<common::Time>>> const& times,
    PopulationOnNetwork const population)
{
	auto const neurons = m_network_graph.get_graph_translation()
	                         .execution_instances.at(population.toExecutionInstanceID())
	                         .spike_labels.at(population);

	assert(
	    times.size() ==
	    std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
	        m_data.data.at(*m_network_graph.get_graph_translation()
	                            .execution_instances.at(population.toExecutionInstanceID())
	                            .event_input_vertex))
	        .size());
	for (size_t b = 0; b < times.size(); ++b) {
		auto const& batch_times = times.at(b);

		if (batch_times.size() != neurons.size()) {
			throw std::runtime_error(
			    "Times number of neurons does not match expected number from spike labels.");
		}

		auto& data_spikes =
		    std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
		        m_data.data.at(*m_network_graph.get_graph_translation()
		                            .execution_instances.at(population.toExecutionInstanceID())
		                            .event_input_vertex))
		        .at(b);

		size_t size = 0;
		for (auto const& ts : batch_times) {
			size += ts.size();
		}

		data_spikes.reserve(size);
		for (size_t i = 0; i < batch_times.size(); ++i) {
			if (!neuron_is_used(m_network_graph, population, i)) {
				continue;
			}
			auto const neuron = neurons.at(i);
			assert(neuron.size() == 1);
			auto const labels = neuron.at(halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron());
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

signal_flow::InputDataSnippet InputGenerator::done()
{
	signal_flow::InputDataSnippet ret;
	for (auto const& [id, execution_instance] :
	     m_network_graph.get_graph_translation().execution_instances) {
		if (!execution_instance.event_input_vertex) {
			continue;
		}
		auto& spikes = std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
		    m_data.data.at(*execution_instance.event_input_vertex));
		for (auto& batch : spikes) {
			std::stable_sort(batch.begin(), batch.end(), [](auto const& a, auto const& b) {
				return a.time < b.time;
			});
		}
		std::vector<signal_flow::TimedSpikeToChipSequence> empty_spikes(spikes.size());
		ret.data.insert({*execution_instance.event_input_vertex, empty_spikes});
	}
	std::swap(ret, m_data);
	return ret;
}

} // namespace grenade::vx::network
