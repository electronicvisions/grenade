#include "grenade/vx/network/extract_output.h"

#include "hate/timer.h"
#include "hate/variant.h"
#include <log4cxx/logger.h>

namespace grenade::vx::network {

std::vector<std::map<
    std::tuple<PopulationOnNetwork, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
    std::vector<common::Time>>>
extract_neuron_spikes(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
{
	hate::Timer timer;
	auto logger = log4cxx::Logger::getLogger("grenade.network.extract_neuron_spikes");

	std::vector<std::map<
	    std::tuple<
	        PopulationOnNetwork, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	    std::vector<common::Time>>>
	    logical_neuron_events(data.batch_size());

	if (network_graph.get_graph_translation().event_output_vertex) {
		// generate reverse lookup table from spike label to neuron coordinate
		std::map<
		    halco::hicann_dls::vx::v3::SpikeLabel,
		    std::tuple<
		        PopulationOnNetwork, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>>
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
						auto const compartment_labels = network_graph.get_graph_translation()
						                                    .spike_labels.at(descriptor)
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
		    data.data.at(*network_graph.get_graph_translation().event_output_vertex));
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


std::vector<std::vector<
    std::tuple<common::Time, AtomicNeuronOnNetwork, haldls::vx::v3::MADCSampleFromChip::Value>>>
extract_madc_samples(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
{
	hate::Timer timer;
	auto logger = log4cxx::Logger::getLogger("grenade.network.extract_madc_samples");
	std::vector<std::vector<
	    std::tuple<common::Time, AtomicNeuronOnNetwork, haldls::vx::v3::MADCSampleFromChip::Value>>>
	    ret(data.batch_size());
	if (network_graph.get_graph_translation().madc_sample_output_vertex) {
		// convert samples
		auto const& samples = std::get<std::vector<signal_flow::TimedMADCSampleFromChipSequence>>(
		    data.data.at(*network_graph.get_graph_translation().madc_sample_output_vertex));
		assert(!samples.size() || samples.size() == ret.size());
		for (size_t b = 0; b < samples.size(); ++b) {
			auto& local_ret = ret.at(b);
			for (auto const& sample : samples.at(b)) {
				auto const& neuron = network_graph.get_network()->madc_recording->neurons.at(
				    sample.data.channel.value());
				local_ret.push_back({sample.time, neuron.coordinate, sample.data.value});
			}
		}
	}
	LOG4CXX_TRACE(logger, "Execution duration: " << timer.print() << ".");
	return ret;
}


std::vector<std::vector<std::tuple<common::Time, AtomicNeuronOnNetwork, signal_flow::Int8>>>
extract_cadc_samples(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
{
	hate::Timer timer;
	auto logger = log4cxx::Logger::getLogger("grenade.network.extract_cadc_samples");
	std::vector<std::vector<
	    std::tuple<common::Time, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, signal_flow::Int8>>>
	    hardware_samples(data.batch_size());
	for (auto const cadc_output_vertex :
	     network_graph.get_graph_translation().cadc_sample_output_vertex) {
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

	std::vector<std::vector<std::tuple<common::Time, AtomicNeuronOnNetwork, signal_flow::Int8>>>
	    ret(hardware_samples.size());

	std::map<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, AtomicNeuronOnNetwork> location_lookup;

	assert(network_graph.get_network());
	if (!network_graph.get_network()->cadc_recording) {
		return ret;
	}
	for (auto const& neuron : network_graph.get_network()->cadc_recording->neurons) {
		auto const atomic_neuron = std::get<Population>(network_graph.get_network()->populations.at(
		                                                    neuron.coordinate.population))
		                               .neurons.at(neuron.coordinate.neuron_on_population)
		                               .coordinate.get_placed_compartments()
		                               .at(neuron.coordinate.compartment_on_neuron)
		                               .at(neuron.coordinate.atomic_neuron_on_compartment);
		location_lookup[atomic_neuron] = neuron.coordinate;
	}

	for (size_t b = 0; b < hardware_samples.size(); ++b) {
		auto const& local_hardware_samples = hardware_samples.at(b);
		auto& local_ret = ret.at(b);

		for (auto const& hardware_sample : local_hardware_samples) {
			auto const& atomic_neuron_on_network = location_lookup.at(std::get<1>(hardware_sample));
			local_ret.push_back(std::tuple{
			    std::get<0>(hardware_sample), atomic_neuron_on_network,
			    std::get<2>(hardware_sample)});
		}
	}
	LOG4CXX_TRACE(logger, "Execution duration: " << timer.print() << ".");
	return ret;
}


namespace {

template <typename T>
void extract_plasticity_rule_recording_data_per_synapse(
    std::map<ProjectionOnNetwork, PlasticityRule::TimedRecordingData::EntryPerSynapse>&
        local_logical_data,
    size_t const num_periods,
    ProjectionOnNetwork const& descriptor,
    NetworkGraph const& network_graph,
    size_t const batch_size,
    size_t const hardware_index,
    size_t const index,
    std::vector<common::TimedDataSequence<std::vector<T>>> const& local_hardware_data)
{
	if (!local_logical_data.contains(descriptor)) {
		std::vector<common::TimedDataSequence<std::vector<std::vector<T>>>>
		    logical_local_hardware_data(batch_size);
		for (size_t b = 0; b < logical_local_hardware_data.size(); ++b) {
			auto& batch = logical_local_hardware_data.at(b);
			batch.resize(num_periods);
			for (auto& sample : batch) {
				sample.data.resize(
				    network_graph.get_network()->projections.at(descriptor).connections.size());
			}
			for (size_t s = 0; s < local_hardware_data.at(b).size(); ++s) {
				batch.at(s).time = local_hardware_data.at(b).at(s).time;
			}
		}
		local_logical_data[descriptor] = logical_local_hardware_data;
	}
	auto& logical_local_hardware_data =
	    std::get<std::vector<common::TimedDataSequence<std::vector<std::vector<T>>>>>(
	        local_logical_data.at(descriptor));
	for (size_t b = 0; b < local_hardware_data.size(); ++b) {
		for (size_t s = 0; s < local_hardware_data.at(b).size(); ++s) {
			auto& local_logical_local_hardware_data = logical_local_hardware_data.at(b).at(s);
			auto& local_local_hardware_data = local_hardware_data.at(b).at(s);
			local_logical_local_hardware_data.data.at(index).push_back(
			    local_local_hardware_data.data.at(hardware_index));
		}
	}
}

template <typename T>
void extract_plasticity_rule_recording_data_per_neuron(
    std::map<PopulationOnNetwork, PlasticityRule::TimedRecordingData::EntryPerNeuron>&
        local_logical_data,
    size_t const num_periods,
    PopulationOnNetwork const& descriptor,
    size_t const batch_size,
    std::vector<std::map<
        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
        std::vector<std::pair<signal_flow::Graph::vertex_descriptor, size_t>>>> const&
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
					for (auto const& [_, atomic_neuron_index] : atomic_neuron_indices) {
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
    PlasticityRuleOnNetwork descriptor)
{
	assert(network_graph.get_network());
	if (!network_graph.get_network()->plasticity_rules.contains(descriptor)) {
		throw std::runtime_error("Requested plasticity rule not part of network graph.");
	}
	if (!network_graph.get_graph_translation().plasticity_rule_output_vertices.contains(
	        descriptor)) {
		throw std::runtime_error("Requested plasticity rule isn't recording data.");
	}
	signal_flow::Graph::vertex_descriptor const output_vertex =
	    network_graph.get_graph_translation().plasticity_rule_output_vertices.at(descriptor);
	if (!data.data.contains(output_vertex)) {
		throw std::runtime_error(
		    "Provided signal_flow::IODataMap doesn't contain data of recording plasticity rule.");
	}
	auto const& output_data =
	    std::get<std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(
	        data.data.at(output_vertex));

	assert(boost::in_degree(output_vertex, network_graph.get_graph().get_graph()) == 1);
	auto const in_edges = boost::in_edges(output_vertex, network_graph.get_graph().get_graph());
	auto const plasticity_rule_vertex =
	    boost::source(*(in_edges.first), network_graph.get_graph().get_graph());

	auto const vertex_recorded_data =
	    std::get<signal_flow::vertex::PlasticityRule>(
	        network_graph.get_graph().get_vertex_property(plasticity_rule_vertex))
	        .extract_recording_data(output_data);
	if (std::holds_alternative<signal_flow::vertex::PlasticityRule::RawRecordingData>(
	        vertex_recorded_data)) {
		return std::get<signal_flow::vertex::PlasticityRule::RawRecordingData>(
		    vertex_recorded_data);
	} else if (!std::holds_alternative<signal_flow::vertex::PlasticityRule::TimedRecordingData>(
	               vertex_recorded_data)) {
		throw std::logic_error("Recording data type not implemented.");
	}

	auto const& vertex_timed_recorded_data =
	    std::get<signal_flow::vertex::PlasticityRule::TimedRecordingData>(vertex_recorded_data);

	PlasticityRule::TimedRecordingData logical_data;
	logical_data.data_array = vertex_timed_recorded_data.data_array;

	auto const& plasticity_rule = network_graph.get_network()->plasticity_rules.at(descriptor);
	auto const batch_size = data.batch_size();

	// create location lookup for data_per_synapse
	// map<vertex_descriptor, IndexOnVertexData>
	std::map<signal_flow::Graph::vertex_descriptor, size_t> data_per_synapse_lookup_table;
	for (size_t index = 0; auto const& in_edge : boost::make_iterator_range(boost::in_edges(
	                           plasticity_rule_vertex, network_graph.get_graph().get_graph()))) {
		auto const source = boost::source(in_edge, network_graph.get_graph().get_graph());
		if (!std::holds_alternative<signal_flow::vertex::SynapseArrayViewSparse>(
		        network_graph.get_graph().get_vertex_property(source))) {
			continue;
		}
		data_per_synapse_lookup_table[source] = index;
		index++;
	}

	auto const extract_observable_per_synapse = [&](auto const& type, auto const& name) {
		typedef typename std::decay_t<decltype(type)>::ElementType ElementType;
		for (auto const& projection : plasticity_rule.projections) {
			for (size_t index = 0;
			     index < network_graph.get_network()->projections.at(projection).connections.size();
			     ++index) {
				auto const& hardware_connections =
				    network_graph.get_graph_translation().projections.at(projection).at(index);
				for (auto const& hardware_index : hardware_connections) {
					auto const& local_result =
					    std::get<std::vector<common::TimedDataSequence<std::vector<ElementType>>>>(
					        vertex_timed_recorded_data.data_per_synapse.at(name).at(
					            data_per_synapse_lookup_table.at(hardware_index.first)));
					extract_plasticity_rule_recording_data_per_synapse(
					    logical_data.data_per_synapse[name],
					    network_graph.get_network()
					        ->plasticity_rules.at(descriptor)
					        .timer.num_periods,
					    projection, network_graph, batch_size, hardware_index.second, index,
					    local_result);
				}
			}
		}
	};

	auto const extract_observable_per_neuron = [&](auto const& type, auto const& name) {
		typedef typename std::decay_t<decltype(type)>::ElementType ElementType;
		for (size_t p = 0; auto const& population : plasticity_rule.populations) {
			std::vector<common::TimedDataSequence<std::vector<ElementType>>> local_record;
			size_t neuron_view_offset = 0;
			auto const neuron_vertices = network_graph.get_graph_translation().neuron_vertices;
			for ([[maybe_unused]] auto const& _ : neuron_vertices.at(population.descriptor)) {
				auto const& local_result =
				    std::get<std::vector<common::TimedDataSequence<std::vector<ElementType>>>>(
				        vertex_timed_recorded_data.data_per_neuron.at(name).at(p));
				local_record.resize(local_result.size());
				for (size_t i = 0; i < local_record.size(); ++i) {
					local_record.at(i).resize(local_result.at(i).size());
					for (size_t s = 0; s < local_record.at(i).size(); ++s) {
						// TODO: make array over hemispheres
						local_record.at(i).at(s).time = local_result.at(i).at(s).time;
						local_record.at(i).at(s).data.resize(
						    std::get<Population>(
						        network_graph.get_network()->populations.at(population.descriptor))
						        .neurons.size());
						for (size_t c = 0; c < local_result.at(i).at(s).data.size(); ++c) {
							local_record.at(i).at(s).data.at(c + neuron_view_offset) =
							    local_result.at(i).at(s).data.at(c);
						}
					}
				}
				if (local_result.size() && local_result.at(0).size()) {
					neuron_view_offset += local_result.at(0).at(0).data.size();
				}
				p++;
			}
			auto const& neuron_translation =
			    network_graph.get_graph_translation().populations.at(population.descriptor);
			extract_plasticity_rule_recording_data_per_neuron(
			    logical_data.data_per_neuron[name],
			    network_graph.get_network()->plasticity_rules.at(descriptor).timer.num_periods,
			    population.descriptor, batch_size, neuron_translation, local_record);
		}
	};

	for (auto const& [name, obsv] :
	     std::get<PlasticityRule::TimedRecording>(*plasticity_rule.recording).observables) {
		std::visit(
		    hate::overloaded{
		        [&](PlasticityRule::TimedRecording::ObservablePerSynapse const& observable) {
			        std::visit(
			            [&](auto const& type) { extract_observable_per_synapse(type, name); },
			            observable.type);
		        },
		        [&](PlasticityRule::TimedRecording::ObservablePerNeuron const& observable) {
			        std::visit(
			            [&](auto const& type) { extract_observable_per_neuron(type, name); },
			            observable.type);
		        },
		        [&](PlasticityRule::TimedRecording::ObservableArray const&) {
			        // handled above, no conversion needed
		        },
		        [&](auto const&) { throw std::logic_error("Observable type not implemented."); }},
		    obsv);
	}
	return logical_data;
}

} // namespace grenade::vx::network
