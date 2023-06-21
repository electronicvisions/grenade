#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_graph_statistics.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/signal_flow/graph.h"
#include "hate/visibility.h"
#include <map>
#include <optional>

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct RoutingResult;

/**
 * Logical network representation.
 */
struct GENPYBIND(visible) NetworkGraph
{
	NetworkGraph() = default;

	/** Underlying network. */
	GENPYBIND(getter_for(network))
	std::shared_ptr<Network> const& get_network() const SYMBOL_VISIBLE;

	/** Graph representing the network. */
	GENPYBIND(getter_for(graph))
	signal_flow::Graph const& get_graph() const SYMBOL_VISIBLE;

	/** Vertex descriptor at which to insert external spike data. */
	GENPYBIND(getter_for(event_input_vertex))
	std::optional<signal_flow::Graph::vertex_descriptor> const& get_event_input_vertex() const
	    SYMBOL_VISIBLE;

	/** Vertex descriptor from which to extract recorded spike data. */
	GENPYBIND(getter_for(event_output_vertex))
	std::optional<signal_flow::Graph::vertex_descriptor> const& get_event_output_vertex() const
	    SYMBOL_VISIBLE;

	/** Vertex descriptor from which to extract recorded madc sample data. */
	GENPYBIND(getter_for(madc_sample_output_vertex))
	std::optional<signal_flow::Graph::vertex_descriptor> const& get_madc_sample_output_vertex()
	    const SYMBOL_VISIBLE;

	/** Vertex descriptor from which to extract recorded cadc sample data. */
	GENPYBIND(getter_for(cadc_sample_output_vertex))
	std::vector<signal_flow::Graph::vertex_descriptor> const& get_cadc_sample_output_vertex() const
	    SYMBOL_VISIBLE;

	/** Vertex descriptors of synapse views. */
	GENPYBIND(getter_for(synapse_vertices))
	std::map<
	    ProjectionDescriptor,
	    std::map<
	        halco::hicann_dls::vx::HemisphereOnDLS,
	        signal_flow::Graph::vertex_descriptor>> const&
	get_synapse_vertices() const SYMBOL_VISIBLE;

	/** Vertex descriptors of neuron views. */
	GENPYBIND(getter_for(neuron_vertices))
	std::map<
	    PopulationDescriptor,
	    std::map<
	        halco::hicann_dls::vx::HemisphereOnDLS,
	        signal_flow::Graph::vertex_descriptor>> const&
	get_neuron_vertices() const SYMBOL_VISIBLE;

	/** Vertex descriptor from which to extract recorded plasticity rule scratchpad memory. */
	GENPYBIND(getter_for(plasticity_rule_output_vertices))
	std::map<PlasticityRuleDescriptor, signal_flow::Graph::vertex_descriptor> const&
	get_plasticity_rule_output_vertices() const SYMBOL_VISIBLE;

	/** Vertex descriptor of plasticity rules. */
	GENPYBIND(getter_for(plasticity_rule_vertices))
	std::map<PlasticityRuleDescriptor, signal_flow::Graph::vertex_descriptor> const&
	get_plasticity_rule_vertices() const SYMBOL_VISIBLE;

	/**
	 * Spike labels corresponding to each neuron in a population.
	 * For external populations these are the input spike labels, for internal population this is
	 * only given for populations with enabled recording.
	 */
	typedef std::map<
	    PopulationDescriptor,
	    std::vector<std::map<
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
	        std::vector<std::optional<halco::hicann_dls::vx::v3::SpikeLabel>>>>>
	    SpikeLabels;
	GENPYBIND(getter_for(spike_labels))
	SpikeLabels const& get_spike_labels() const SYMBOL_VISIBLE;

	/**
	 * Translation between unrouted and routed graph representation.
	 */
	struct GraphTranslation
	{
		/**
		 * Translation of Projection::Connection to synapses in signal-flow graph.
		 * For each projection (descriptor) a vector with equal ordering to the connections of the
		 * projection contains possibly multiple signal-flow graph vertex descriptors and indices on
		 * synapses on the signal-flow graph vertex property.
		 */
		typedef std::map<
		    ProjectionDescriptor,
		    std::vector<std::vector<std::pair<signal_flow::Graph::vertex_descriptor, size_t>>>>
		    Projections;
		Projections projections;

		/**
		 * Translation of Population neurons to (multiple) atomic neurons in signal-flow graph.
		 * For each population (descriptor) a vector with equal ordering to the neurons of the
		 * population contains for each compartment on the logical neuron and for each atomic neuron
		 * on the logical neuron compartment a pair of signal-flow graph vertex descriptor and index
		 * of the corresponding atomic neuron on the signal-flow graph vertex property.
		 */
		typedef std::map<
		    PopulationDescriptor,
		    std::vector<std::map<
		        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
		        std::vector<std::pair<signal_flow::Graph::vertex_descriptor, size_t>>>>>
		    Populations;
		Populations populations;
	};
	/** Translation between unrouted and routed graph representation. */
	GENPYBIND(getter_for(graph_translation))
	GraphTranslation const& get_graph_translation() const SYMBOL_VISIBLE;

	/**
	 * Checks validity of hardware graph representation in relation to the abstract network.
	 * This ensures all required elements and information being present as well as a functionally
	 * correct mapping and routing.
	 */
	bool valid() const SYMBOL_VISIBLE;

	/*
	 * Placed connection in synapse matrix.
	 */
	struct PlacedConnection
	{
		/** Weight of connection. */
		lola::vx::v3::SynapseMatrix::Weight weight;
		/** Vertical location. */
		halco::hicann_dls::vx::v3::SynapseRowOnDLS synapse_row;
		/** Horizontal location. */
		halco::hicann_dls::vx::v3::SynapseOnSynapseRow synapse_on_row;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream&, PlacedConnection const&) SYMBOL_VISIBLE;
	};

	typedef std::vector<std::vector<PlacedConnection>> PlacedConnections;

	std::vector<PlacedConnection> get_placed_connection(
	    ProjectionDescriptor descriptor, size_t index) const SYMBOL_VISIBLE;
	PlacedConnections get_placed_connections(ProjectionDescriptor descriptor) const SYMBOL_VISIBLE;

private:
	std::shared_ptr<Network> m_network;

	signal_flow::Graph m_graph;
	std::optional<signal_flow::Graph::vertex_descriptor> m_event_input_vertex;
	std::optional<signal_flow::Graph::vertex_descriptor> m_event_output_vertex;
	std::optional<signal_flow::Graph::vertex_descriptor> m_madc_sample_output_vertex;
	std::vector<signal_flow::Graph::vertex_descriptor> m_cadc_sample_output_vertex;
	std::map<
	    ProjectionDescriptor,
	    std::map<halco::hicann_dls::vx::HemisphereOnDLS, signal_flow::Graph::vertex_descriptor>>
	    m_synapse_vertices;
	std::map<
	    PopulationDescriptor,
	    std::map<halco::hicann_dls::vx::HemisphereOnDLS, signal_flow::Graph::vertex_descriptor>>
	    m_neuron_vertices;
	std::map<
	    PopulationDescriptor,
	    std::map<halco::hicann_dls::vx::HemisphereOnDLS, signal_flow::Graph::vertex_descriptor>>
	    m_background_spike_source_vertices;
	std::map<PlasticityRuleDescriptor, signal_flow::Graph::vertex_descriptor>
	    m_plasticity_rule_vertices;
	std::map<PlasticityRuleDescriptor, signal_flow::Graph::vertex_descriptor>
	    m_plasticity_rule_output_vertices;
	SpikeLabels m_spike_labels;

	GraphTranslation m_graph_translation;

	std::chrono::microseconds m_construction_duration;
	std::chrono::microseconds m_verification_duration;
	std::chrono::microseconds m_routing_duration;

	friend NetworkGraph build_network_graph(
	    std::shared_ptr<Network> const& network,
	    RoutingResult const& routing_result,
	    signal_flow::ExecutionInstance const& execution_instance);
	friend void update_network_graph(
	    NetworkGraph& network_graph, std::shared_ptr<Network> const& network);
	friend NetworkGraphStatistics extract_statistics(NetworkGraph const& network_graph);
};

} // namespace grenade::vx::network
