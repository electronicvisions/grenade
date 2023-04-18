#include "grenade/vx/network/placed_logical/network_graph.h"

namespace grenade::vx::network::placed_logical {

std::shared_ptr<Network> const& NetworkGraph::get_network() const
{
	return m_network;
}

std::shared_ptr<network::placed_atomic::Network> const& NetworkGraph::get_hardware_network() const
{
	return m_hardware_network_graph.get_network();
}

ConnectionRoutingResult const& NetworkGraph::get_connection_routing_result() const
{
	return m_connection_routing_result;
}

placed_atomic::NetworkGraph const& NetworkGraph::get_hardware_network_graph() const
{
	return m_hardware_network_graph;
}

signal_flow::Graph const& NetworkGraph::get_graph() const
{
	return m_hardware_network_graph.get_graph();
}

NetworkGraph::PopulationTranslation const& NetworkGraph::get_population_translation() const
{
	return m_population_translation;
}

NetworkGraph::NeuronTranslation const& NetworkGraph::get_neuron_translation() const
{
	return m_neuron_translation;
}

NetworkGraph::ProjectionTranslation const& NetworkGraph::get_projection_translation() const
{
	return m_projection_translation;
}

NetworkGraph::PlasticityRuleTranslation const& NetworkGraph::get_plasticity_rule_translation() const
{
	return m_plasticity_rule_translation;
}

std::optional<signal_flow::Graph::vertex_descriptor> NetworkGraph::get_event_input_vertex() const
{
	return m_hardware_network_graph.get_event_input_vertex();
}

std::optional<signal_flow::Graph::vertex_descriptor> NetworkGraph::get_event_output_vertex() const
{
	return m_hardware_network_graph.get_event_output_vertex();
}

std::optional<signal_flow::Graph::vertex_descriptor> NetworkGraph::get_madc_sample_output_vertex()
    const
{
	return m_hardware_network_graph.get_madc_sample_output_vertex();
}

std::vector<signal_flow::Graph::vertex_descriptor> NetworkGraph::get_cadc_sample_output_vertex()
    const
{
	return m_hardware_network_graph.get_cadc_sample_output_vertex();
}

std::map<
    ProjectionDescriptor,
    std::map<halco::hicann_dls::vx::HemisphereOnDLS, signal_flow::Graph::vertex_descriptor>>
NetworkGraph::get_synapse_vertices() const
{
	std::map<
	    ProjectionDescriptor,
	    std::map<halco::hicann_dls::vx::HemisphereOnDLS, signal_flow::Graph::vertex_descriptor>>
	    ret;
	for (auto const& [atomic_descriptor, vertices] :
	     m_hardware_network_graph.get_synapse_vertices()) {
		ret[ProjectionDescriptor(atomic_descriptor.value())] = vertices;
	}
	return ret;
}

std::map<
    PopulationDescriptor,
    std::map<halco::hicann_dls::vx::HemisphereOnDLS, signal_flow::Graph::vertex_descriptor>>
NetworkGraph::get_neuron_vertices() const
{
	std::map<
	    PopulationDescriptor,
	    std::map<halco::hicann_dls::vx::HemisphereOnDLS, signal_flow::Graph::vertex_descriptor>>
	    ret;
	for (auto const& [atomic_descriptor, vertices] :
	     m_hardware_network_graph.get_neuron_vertices()) {
		ret[std::find_if(
		        m_population_translation.begin(), m_population_translation.end(),
		        [atomic_descriptor](auto const& item) { return item.second == atomic_descriptor; })
		        ->first] = vertices;
	}
	return ret;
}

std::map<PlasticityRuleDescriptor, signal_flow::Graph::vertex_descriptor>
NetworkGraph::get_plasticity_rule_output_vertices() const
{
	std::map<PlasticityRuleDescriptor, signal_flow::Graph::vertex_descriptor> ret;
	for (auto const& [atomic_descriptor, vertex] :
	     m_hardware_network_graph.get_plasticity_rule_output_vertices()) {
		ret[PlasticityRuleDescriptor(atomic_descriptor.value())] = vertex;
	}
	return ret;
}

NetworkGraph::SpikeLabels NetworkGraph::get_spike_labels() const
{
	auto const atomic_spike_labels = m_hardware_network_graph.get_spike_labels();

	SpikeLabels spike_labels;
	assert(m_network);
	for (auto const& [descriptor, population] : m_network->populations) {
		auto& local_spike_labels = spike_labels[descriptor];
		auto const& local_atomic_spike_labels =
		    atomic_spike_labels.at(m_population_translation.at(descriptor));
		auto const& local_neuron_translation = m_neuron_translation.at(descriptor);
		for (size_t n = 0; auto const& neuron : local_neuron_translation) {
			std::map<
			    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
			    std::vector<std::optional<halco::hicann_dls::vx::v3::SpikeLabel>>>
			    neuron_spike_labels;
			for (auto const& [compartment, an_translations] : neuron) {
				auto& compartment_spike_labels = neuron_spike_labels[compartment];
				if (std::holds_alternative<Population>(population)) {
					auto const& pop = std::get<Population>(population);
					auto const& spike_master =
					    pop.neurons.at(n).compartments.at(compartment).spike_master;
					for (size_t an = 0; an < an_translations.size(); ++an) {
						if (spike_master && (spike_master->neuron_on_compartment == an)) {
							assert(
							    local_atomic_spike_labels
							        .at(an_translations.at(spike_master->neuron_on_compartment))
							        .size() == 1);
							compartment_spike_labels.push_back(
							    local_atomic_spike_labels
							        .at(an_translations.at(spike_master->neuron_on_compartment))
							        .at(0));
						} else {
							compartment_spike_labels.push_back(std::nullopt);
						}
					}
				} else {
					assert(an_translations.size() == 1);
					compartment_spike_labels = local_atomic_spike_labels.at(an_translations.at(0));
				}
			}
			local_spike_labels.push_back(neuron_spike_labels);
			n++;
		}
	}
	return spike_labels;
}

NetworkGraph::GraphTranslation const& NetworkGraph::get_graph_translation() const
{
	return m_graph_translation;
}

bool NetworkGraph::valid() const
{
	// TODO: check logical to atomic mapping validity
	return m_hardware_network_graph.valid();
}

std::vector<NetworkGraph::PlacedConnection> NetworkGraph::get_placed_connection(
    ProjectionDescriptor const descriptor, size_t const index) const
{
	std::vector<NetworkGraph::PlacedConnection> ret;
	for (auto const& [_, atomic] :
	     boost::make_iterator_range(m_projection_translation.equal_range({descriptor, index}))) {
		auto const& [atomic_descriptor, atomic_index] = atomic;
		ret.push_back(
		    m_hardware_network_graph.get_placed_connection(atomic_descriptor, atomic_index));
	}
	return ret;
}

NetworkGraph::PlacedConnections NetworkGraph::get_placed_connections(
    ProjectionDescriptor const descriptor) const
{
	PlacedConnections placed_connections;
	assert(m_network);
	for (size_t i = 0; i < m_network->projections.at(descriptor).connections.size(); ++i) {
		placed_connections.push_back(get_placed_connection(descriptor, i));
	}
	return placed_connections;
}

} // namespace grenade::vx::network::placed_logical
