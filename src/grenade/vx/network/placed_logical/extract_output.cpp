#include "grenade/vx/network/placed_logical/extract_output.h"

#include "grenade/vx/network/placed_atomic/extract_output.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <log4cxx/logger.h>

namespace grenade::vx::network::placed_logical {

std::vector<std::map<
    std::tuple<PopulationDescriptor, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
    std::vector<common::Time>>>
extract_neuron_spikes(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
{
	hate::Timer timer;
	auto logger =
	    log4cxx::Logger::getLogger("grenade.network.placed_logical.extract_neuron_spikes");

	std::vector<std::map<
	    std::tuple<
	        PopulationDescriptor, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	    std::vector<common::Time>>>
	    logical_neuron_events(data.batch_size());

	if (network_graph.get_event_output_vertex()) {
		// generate reverse lookup table from spike label to neuron coordinate
		std::map<
		    halco::hicann_dls::vx::v3::SpikeLabel,
		    std::tuple<
		        PopulationDescriptor, size_t,
		        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>>
		    label_lookup;
		assert(network_graph.get_network());
		for (auto const& [descriptor, population] : network_graph.get_network()->populations) {
			if (!std::holds_alternative<Population>(population)) {
				continue;
			}
			size_t i = 0;
			for (auto const& neuron : std::get<Population>(population).neurons) {
				for (auto const& [compartment_on_neuron, compartment] : neuron.compartments) {
					if (compartment.spike_master &&
					    compartment.spike_master->enable_record_spikes) {
						auto const compartment_labels = network_graph.get_spike_labels()
						                                    .at(descriptor)
						                                    .at(i)
						                                    .at(compartment_on_neuron);
						// internal neurons only have one label assigned
						assert(
						    compartment_labels.size() == neuron.coordinate.get_placed_compartments()
						                                     .at(compartment_on_neuron)
						                                     .size());
						assert(
						    compartment_labels.at(compartment.spike_master->neuron_on_compartment));
						label_lookup[*compartment_labels.at(
						    compartment.spike_master->neuron_on_compartment)] =
						    std::tuple{descriptor, i, compartment_on_neuron};
					}
				}
				i++;
			}
		}

		// convert spikes
		auto const& spikes = std::get<std::vector<signal_flow::TimedSpikeFromChipSequence>>(
		    data.data.at(*network_graph.get_event_output_vertex()));
		assert(!spikes.size() || spikes.size() == data.batch_size());

		for (size_t batch = 0; batch < spikes.size(); ++batch) {
			auto& local_logical_neuron_events = logical_neuron_events.at(batch);
			for (auto const& spike : spikes.at(batch)) {
				if (label_lookup.contains(spike.data)) {
					local_logical_neuron_events[label_lookup.at(spike.data)].push_back(spike.time);
				}
			}
		}
	}

	LOG4CXX_TRACE(logger, "Execution duration: " << timer.print() << ".");
	return logical_neuron_events;
}


std::vector<std::vector<std::pair<common::Time, haldls::vx::v3::MADCSampleFromChip::Value>>>
extract_madc_samples(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
{
	hate::Timer timer;
	auto logger = log4cxx::Logger::getLogger("grenade.network.placed_logical.extract_madc_samples");
	if (!network_graph.get_madc_sample_output_vertex()) {
		std::vector<std::vector<std::pair<common::Time, haldls::vx::v3::MADCSampleFromChip::Value>>>
		    ret(data.batch_size());
		return ret;
	}
	// convert samples
	auto const& samples = std::get<std::vector<signal_flow::TimedMADCSampleFromChipSequence>>(
	    data.data.at(*network_graph.get_madc_sample_output_vertex()));
	std::vector<std::vector<std::pair<common::Time, haldls::vx::v3::MADCSampleFromChip::Value>>>
	    ret(data.batch_size());
	assert(!samples.size() || samples.size() == data.batch_size());
	for (size_t b = 0; b < samples.size(); ++b) {
		auto& local_ret = ret.at(b);
		for (auto const& sample : samples.at(b)) {
			local_ret.push_back({sample.time, sample.data});
		}
	}
	LOG4CXX_TRACE(logger, "Execution duration: " << timer.print() << ".");
	return ret;
}


std::vector<std::vector<std::tuple<
    common::Time,
    PopulationDescriptor,
    size_t,
    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
    size_t,
    signal_flow::Int8>>>
extract_cadc_samples(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
{
	hate::Timer timer;
	auto logger = log4cxx::Logger::getLogger("grenade.network.placed_logical.extract_cadc_samples");
	std::vector<std::vector<
	    std::tuple<common::Time, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, signal_flow::Int8>>>
	    hardware_samples(data.batch_size());
	for (auto const cadc_output_vertex : network_graph.get_cadc_sample_output_vertex()) {
		auto const& samples =
		    std::get<std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(
		        data.data.at(cadc_output_vertex));
		assert(!samples.size() || samples.size() == data.batch_size());
		assert(boost::in_degree(cadc_output_vertex, network_graph.get_graph().get_graph()) == 1);
		auto const in_edges =
		    boost::in_edges(cadc_output_vertex, network_graph.get_graph().get_graph());
		auto const cadc_vertex =
		    boost::source(*in_edges.first, network_graph.get_graph().get_graph());
		auto const& vertex = std::get<signal_flow::vertex::CADCMembraneReadoutView>(
		    network_graph.get_graph().get_vertex_property(cadc_vertex));
		auto const& columns = vertex.get_columns();
		auto const& row = vertex.get_synram().toNeuronRowOnDLS();
		for (size_t b = 0; b < samples.size(); ++b) {
			auto& local_ret = hardware_samples.at(b);
			for (auto const& sample : samples.at(b)) {
				for (size_t j = 0; auto const& cs : columns) {
					for (auto const& column : cs) {
						local_ret.push_back(
						    {sample.time,
						     halco::hicann_dls::vx::v3::AtomicNeuronOnDLS(
						         column.toNeuronColumnOnDLS(), row),
						     sample.data.at(j)});
						j++;
					}
				}
			}
		}
	}
	for (auto& batch : hardware_samples) {
		std::sort(batch.begin(), batch.end());
	}

	std::vector<std::vector<std::tuple<
	    common::Time, PopulationDescriptor, size_t,
	    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, size_t, signal_flow::Int8>>>
	    ret(hardware_samples.size());

	std::map<
	    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS,
	    std::tuple<
	        PopulationDescriptor, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
	        size_t>>
	    location_lookup;

	assert(network_graph.get_network());
	if (!network_graph.get_network()->cadc_recording) {
		return ret;
	}
	for (auto const& neuron : network_graph.get_network()->cadc_recording->neurons) {
		auto const atomic_neuron =
		    std::get<Population>(network_graph.get_network()->populations.at(neuron.population))
		        .neurons.at(neuron.neuron_on_population)
		        .coordinate.get_placed_compartments()
		        .at(neuron.compartment_on_neuron)
		        .at(neuron.atomic_neuron_on_compartment);
		location_lookup[atomic_neuron] = std::tuple{
		    neuron.population, neuron.neuron_on_population, neuron.compartment_on_neuron,
		    neuron.atomic_neuron_on_compartment};
	}

	for (size_t b = 0; b < hardware_samples.size(); ++b) {
		auto const& local_hardware_samples = hardware_samples.at(b);
		auto& local_ret = ret.at(b);

		for (auto const& hardware_sample : local_hardware_samples) {
			auto const& [population, neuron_on_population, compartment_on_neuron, atomic_neuron_on_compartment] =
			    location_lookup.at(std::get<1>(hardware_sample));
			local_ret.push_back(std::tuple{
			    std::get<0>(hardware_sample), population, neuron_on_population,
			    compartment_on_neuron, atomic_neuron_on_compartment, std::get<2>(hardware_sample)});
		}
	}
	LOG4CXX_TRACE(logger, "Execution duration: " << timer.print() << ".");
	return ret;
}


namespace {

template <typename T>
void extract_plasticity_rule_recording_data_per_synapse(
    std::map<ProjectionDescriptor, PlasticityRule::TimedRecordingData::EntryPerSynapse>&
        local_logical_data,
    size_t const num_periods,
    ProjectionDescriptor const& descriptor,
    NetworkGraph const& network_graph,
    size_t const batch_size,
    size_t const hardware_index,
    size_t const index,
    std::vector<common::TimedDataSequence<std::vector<T>>> const& local_signal_flow)
{
	if (!local_logical_data.contains(descriptor)) {
		std::vector<common::TimedDataSequence<std::vector<std::vector<T>>>>
		    logical_local_signal_flow(batch_size);
		for (size_t b = 0; b < logical_local_signal_flow.size(); ++b) {
			auto& batch = logical_local_signal_flow.at(b);
			batch.resize(num_periods);
			for (auto& sample : batch) {
				sample.data.resize(
				    network_graph.get_network()->projections.at(descriptor).connections.size());
			}
			for (size_t s = 0; s < local_signal_flow.at(b).size(); ++s) {
				batch.at(s).time = local_signal_flow.at(b).at(s).time;
			}
		}
		local_logical_data[descriptor] = logical_local_signal_flow;
	}
	auto& logical_local_signal_flow =
	    std::get<std::vector<common::TimedDataSequence<std::vector<std::vector<T>>>>>(
	        local_logical_data.at(descriptor));
	for (size_t b = 0; b < local_signal_flow.size(); ++b) {
		for (size_t s = 0; s < local_signal_flow.at(b).size(); ++s) {
			auto& local_logical_local_signal_flow = logical_local_signal_flow.at(b).at(s);
			auto& local_local_signal_flow = local_signal_flow.at(b).at(s);
			local_logical_local_signal_flow.data.at(index).push_back(
			    local_local_signal_flow.data.at(hardware_index));
		}
	}
}

template <typename T>
void extract_plasticity_rule_recording_data_per_neuron(
    std::map<PopulationDescriptor, PlasticityRule::TimedRecordingData::EntryPerNeuron>&
        local_logical_data,
    size_t const num_periods,
    PopulationDescriptor const& descriptor,
    size_t const batch_size,
    std::vector<
        std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, std::vector<size_t>>> const&
        neuron_translation,
    std::vector<common::TimedDataSequence<std::vector<T>>> const& dd)
{
	if (!local_logical_data.contains(descriptor)) {
		// dimension (outer to inner): batch, time, neuron_on_population, compartment_on_neuron,
		// atomic_neuron_on_compartment
		std::vector<common::TimedDataSequence<std::vector<
		    std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, std::vector<T>>>>>
		    logical_dd(batch_size);
		for (size_t b = 0; b < logical_dd.size(); ++b) {
			auto& batch = logical_dd.at(b);
			batch.resize(num_periods);
			for (auto& sample : batch) {
				sample.data.resize(neuron_translation.size());
			}
			for (size_t s = 0; s < dd.at(b).size(); ++s) {
				batch.at(s).time = dd.at(b).at(s).time;
			}
		}
		local_logical_data[descriptor] = logical_dd;
	}
	auto& logical_dd = std::get<std::vector<common::TimedDataSequence<std::vector<
	    std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, std::vector<T>>>>>>(
	    local_logical_data.at(descriptor));
	for (size_t b = 0; b < dd.size(); ++b) {
		for (size_t s = 0; s < dd.at(b).size(); ++s) {
			auto& local_logical_dd = logical_dd.at(b).at(s);
			auto& local_dd = dd.at(b).at(s);
			for (size_t neuron = 0; neuron < neuron_translation.size(); ++neuron) {
				auto& neuron_data = local_logical_dd.data.at(neuron);
				for (auto const& [compartment, atomic_neuron_indices] :
				     neuron_translation.at(neuron)) {
					auto& compartment_data = neuron_data[compartment];
					for (auto const& atomic_neuron_index : atomic_neuron_indices) {
						compartment_data.push_back(local_dd.data.at(atomic_neuron_index));
					}
				}
			}
		}
	}
}

} // namespace

PlasticityRule::RecordingData extract_plasticity_rule_recording_data(
    signal_flow::IODataMap const& data,
    NetworkGraph const& network_graph,
    PlasticityRuleDescriptor descriptor)
{
	auto const& hardware_network_graph = network_graph.get_hardware_network_graph();
	auto const signal_flow = network::placed_atomic::extract_plasticity_rule_recording_data(
	    data, hardware_network_graph,
	    network_graph.get_plasticity_rule_translation().at(descriptor));

	auto const batch_size = data.batch_size();

	auto const convert_raw =
	    [](network::placed_atomic::PlasticityRule::RawRecordingData const& data)
	    -> PlasticityRule::RecordingData { return data; };

	auto const convert_timed =
	    [batch_size, descriptor,
	     network_graph](network::placed_atomic::PlasticityRule::TimedRecordingData const& data)
	    -> PlasticityRule::RecordingData {
		PlasticityRule::TimedRecordingData logical_data;
		for (auto const& logical_proj :
		     network_graph.get_network()->plasticity_rules.at(descriptor).projections) {
			for (auto const& [name, d] : data.data_per_synapse) {
				auto& local_logical_data = logical_data.data_per_synapse[name];
				for (size_t index = 0;
				     index <
				     network_graph.get_network()->projections.at(logical_proj).connections.size();
				     ++index) {
					auto const hardware_connections =
					    network_graph.get_projection_translation().equal_range(
					        std::make_pair(logical_proj, index));
					for (auto const& [_, hardware_index] :
					     boost::make_iterator_range(hardware_connections)) {
						auto const extractor = [&](auto const& dd) {
							extract_plasticity_rule_recording_data_per_synapse(
							    local_logical_data,
							    network_graph.get_network()
							        ->plasticity_rules.at(descriptor)
							        .timer.num_periods,
							    logical_proj, network_graph, batch_size, hardware_index.second,
							    index, dd);
						};
						std::visit(extractor, d.at(hardware_index.first));
					}
				}
			}
		}
		for (auto const& logical_pop :
		     network_graph.get_network()->plasticity_rules.at(descriptor).populations) {
			for (auto const& [name, d] : data.data_per_neuron) {
				auto& local_logical_data = logical_data.data_per_neuron[name];
				auto const& neuron_translation =
				    network_graph.get_neuron_translation().at(logical_pop.descriptor);
				auto const extractor = [&](auto const& dd) {
					extract_plasticity_rule_recording_data_per_neuron(
					    local_logical_data,
					    network_graph.get_network()
					        ->plasticity_rules.at(descriptor)
					        .timer.num_periods,
					    logical_pop.descriptor, batch_size, neuron_translation, dd);
				};
				std::visit(
				    extractor,
				    d.at(network_graph.get_population_translation().at(logical_pop.descriptor)));
			}
		}
		logical_data.data_array = data.data_array;
		return logical_data;
	};

	return std::visit(hate::overloaded{convert_raw, convert_timed}, signal_flow);
}

} // namespace grenade::vx::network::placed_logical
