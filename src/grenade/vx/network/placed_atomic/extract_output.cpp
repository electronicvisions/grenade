#include "grenade/vx/network/placed_atomic/extract_output.h"

#include "halco/hicann-dls/vx/v3/event.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <log4cxx/logger.h>

namespace grenade::vx::network::placed_atomic {

std::vector<std::map<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, std::vector<common::Time>>>
extract_neuron_spikes(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
{
	hate::Timer timer;
	auto logger = log4cxx::Logger::getLogger("grenade.network.extract_neuron_spikes");
	if (!network_graph.get_event_output_vertex()) {
		return std::vector<
		    std::map<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, std::vector<common::Time>>>(
		    data.batch_size());
	}
	// generate reverse lookup table from spike label to neuron coordinate
	std::map<halco::hicann_dls::vx::v3::SpikeLabel, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>
	    label_lookup;
	assert(network_graph.get_network());
	for (auto const& [descriptor, neurons] : network_graph.get_spike_labels()) {
		if (!std::holds_alternative<Population>(
		        network_graph.get_network()->populations.at(descriptor))) {
			continue;
		}
		auto const& population =
		    std::get<Population>(network_graph.get_network()->populations.at(descriptor));
		for (size_t i = 0; i < neurons.size(); ++i) {
			if (population.enable_record_spikes.at(i)) {
				// internal neurons only have one label assigned
				assert(neurons.at(i).size() == 1);
				assert(neurons.at(i).at(0));
				label_lookup[*(neurons.at(i).at(0))] = population.neurons.at(i);
			}
		}
	}
	// convert spikes
	auto const& spikes = std::get<std::vector<signal_flow::TimedSpikeFromChipSequence>>(
	    data.data.at(*network_graph.get_event_output_vertex()));
	std::vector<std::map<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, std::vector<common::Time>>>
	    ret(data.batch_size());
	assert(!spikes.size() || spikes.size() == data.batch_size());
	for (size_t b = 0; b < spikes.size(); ++b) {
		auto& local_ret = ret.at(b);
		for (auto const& spike : spikes.at(b)) {
			auto const label = spike.data;
			if (label_lookup.contains(label)) {
				local_ret[label_lookup.at(label)].push_back(spike.time);
			}
		}
	}
	LOG4CXX_TRACE(logger, "Execution duration: " << timer.print() << ".");
	return ret;
}

std::vector<std::vector<std::pair<common::Time, haldls::vx::v3::MADCSampleFromChip::Value>>>
extract_madc_samples(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
{
	hate::Timer timer;
	auto logger = log4cxx::Logger::getLogger("grenade.network.extract_madc_samples");
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

std::vector<std::vector<
    std::tuple<common::Time, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, signal_flow::Int8>>>
extract_cadc_samples(signal_flow::IODataMap const& data, NetworkGraph const& network_graph)
{
	hate::Timer timer;
	auto logger = log4cxx::Logger::getLogger("grenade.network.extract_cadc_samples");
	// convert samples
	std::vector<std::vector<
	    std::tuple<common::Time, halco::hicann_dls::vx::v3::AtomicNeuronOnDLS, signal_flow::Int8>>>
	    ret(data.batch_size());
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
			auto& local_ret = ret.at(b);
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
	for (auto& batch : ret) {
		std::sort(batch.begin(), batch.end());
	}
	LOG4CXX_TRACE(logger, "Execution duration: " << timer.print() << ".");
	return ret;
}

PlasticityRule::RecordingData extract_plasticity_rule_recording_data(
    signal_flow::IODataMap const& data,
    NetworkGraph const& network_graph,
    PlasticityRuleDescriptor const descriptor)
{
	assert(network_graph.get_network());
	if (!network_graph.get_network()->plasticity_rules.contains(descriptor)) {
		throw std::runtime_error("Requested plasticity rule not part of network graph.");
	}
	if (!network_graph.get_plasticity_rule_output_vertices().contains(descriptor)) {
		throw std::runtime_error("Requested plasticity rule isn't recording data.");
	}
	signal_flow::Graph::vertex_descriptor const output_vertex =
	    network_graph.get_plasticity_rule_output_vertices().at(descriptor);
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

	PlasticityRule::TimedRecordingData recorded_data;
	recorded_data.data_array = vertex_timed_recorded_data.data_array;

	auto const& plasticity_rule = network_graph.get_network()->plasticity_rules.at(descriptor);

	auto const extract_observable_per_synapse = [&](auto const& type, auto const& name) {
		typedef typename std::decay_t<decltype(type)>::ElementType ElementType;
		for (size_t p = 0; auto const& projection : plasticity_rule.projections) {
			recorded_data.data_per_synapse[name][projection] =
			    std::vector<common::TimedDataSequence<std::vector<ElementType>>>{};
			auto& local_record =
			    std::get<std::vector<common::TimedDataSequence<std::vector<ElementType>>>>(
			        recorded_data.data_per_synapse.at(name).at(projection));
			size_t synapse_view_offset = 0;
			for ([[maybe_unused]] auto const& _ :
			     network_graph.get_synapse_vertices().at(projection)) {
				auto const& local_result =
				    std::get<std::vector<common::TimedDataSequence<std::vector<ElementType>>>>(
				        vertex_timed_recorded_data.data_per_synapse.at(name).at(p));
				local_record.resize(local_result.size());
				for (size_t i = 0; i < local_record.size(); ++i) {
					local_record.at(i).resize(local_result.at(i).size());
					for (size_t s = 0; s < local_record.at(i).size(); ++s) {
						// TODO: make array over hemispheres
						local_record.at(i).at(s).time = local_result.at(i).at(s).time;
						local_record.at(i).at(s).data.resize(network_graph.get_network()
						                                         ->projections.at(projection)
						                                         .connections.size());
						for (size_t c = 0; c < local_result.at(i).at(s).data.size(); ++c) {
							local_record.at(i).at(s).data.at(c + synapse_view_offset) =
							    local_result.at(i).at(s).data.at(c);
						}
					}
				}
				if (local_result.size() && local_result.at(0).size()) {
					synapse_view_offset += local_result.at(0).at(0).data.size();
				}
				p++;
			}
		}
	};

	auto const extract_observable_per_neuron = [&](auto const& type, auto const& name) {
		typedef typename std::decay_t<decltype(type)>::ElementType ElementType;
		for (size_t p = 0; auto const& population : plasticity_rule.populations) {
			recorded_data.data_per_neuron[name][population.descriptor] =
			    std::vector<common::TimedDataSequence<std::vector<ElementType>>>{};
			auto& local_record =
			    std::get<std::vector<common::TimedDataSequence<std::vector<ElementType>>>>(
			        recorded_data.data_per_neuron.at(name).at(population.descriptor));
			size_t neuron_view_offset = 0;
			for ([[maybe_unused]] auto const& _ :
			     network_graph.get_neuron_vertices().at(population.descriptor)) {
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
		}
	};

	for (auto const& [name, obsv] :
	     std::get<network::placed_atomic::PlasticityRule::TimedRecording>(
	         *plasticity_rule.recording)
	         .observables) {
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
	return recorded_data;
}

} // namespace grenade::vx::network::placed_atomic
