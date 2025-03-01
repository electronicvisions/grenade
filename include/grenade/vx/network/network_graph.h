#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_graph_statistics.h"
#include "grenade/vx/network/population_on_network.h"
#include "grenade/vx/network/projection_on_network.h"
#include "grenade/vx/signal_flow/graph.h"
#include "hate/visibility.h"
#include <map>
#include <optional>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

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

	/**
	 * Translation between unrouted and routed graph representation.
	 */
	struct GraphTranslation
	{
		struct ExecutionInstance
		{
			/** Vertex descriptor at which to insert external spike data. */
			std::optional<signal_flow::Graph::vertex_descriptor> event_input_vertex;

			/** Vertex descriptor which merges external spike data. */
			std::optional<signal_flow::Graph::vertex_descriptor> event_merger_vertex;

			/** Vertex descriptor from which to extract recorded spike data. */
			std::optional<signal_flow::Graph::vertex_descriptor> event_output_vertex;

			/** Vertex descriptor from which to extract recorded madc sample data. */
			std::optional<signal_flow::Graph::vertex_descriptor> madc_sample_output_vertex;

			/** Vertex descriptor from which to extract recorded cadc sample data. */
			std::vector<signal_flow::Graph::vertex_descriptor> cadc_sample_output_vertex;

			/** Vertex descriptor of pad readouts. */
			std::vector<signal_flow::Graph::vertex_descriptor> pad_readout_vertex;

			/** Vertex descriptors of synapse views. */
			std::map<
			    ProjectionOnExecutionInstance,
			    std::map<
			        halco::hicann_dls::vx::HemisphereOnDLS,
			        signal_flow::Graph::vertex_descriptor>>
			    synapse_vertices;

			/** Vertex descriptors of neuron views. */
			std::map<
			    PopulationOnExecutionInstance,
			    std::map<
			        halco::hicann_dls::vx::HemisphereOnDLS,
			        signal_flow::Graph::vertex_descriptor>>
			    neuron_vertices;

			/** Vertex descriptors of background spike sources. */
			std::map<
			    PopulationOnExecutionInstance,
			    std::map<
			        halco::hicann_dls::vx::HemisphereOnDLS,
			        signal_flow::Graph::vertex_descriptor>>
			    background_spike_source_vertices;

			/** Vertex descriptor from which to extract recorded plasticity rule scratchpad memory.
			 */
			std::map<PlasticityRuleOnExecutionInstance, signal_flow::Graph::vertex_descriptor>
			    plasticity_rule_output_vertices;

			/** Vertex descriptor of plasticity rules. */
			std::map<PlasticityRuleOnExecutionInstance, signal_flow::Graph::vertex_descriptor>
			    plasticity_rule_vertices;

			/**
			 * Spike labels corresponding to each neuron in a population.
			 * For external populations these are the input spike labels, for internal population
			 * this is only given for populations with enabled recording.
			 */
			typedef std::map<
			    PopulationOnExecutionInstance,
			    std::vector<std::map<
			        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
			        std::vector<std::optional<halco::hicann_dls::vx::v3::SpikeLabel>>>>>
			    SpikeLabels;
			SpikeLabels spike_labels;

			/**
			 * Translation of Projection::Connection to synapses in signal-flow graph.
			 * For each projection (descriptor) a vector with equal ordering to the connections of
			 * the projection contains possibly multiple signal-flow graph vertex descriptors and
			 * indices on synapses on the signal-flow graph vertex property.
			 */
			typedef std::map<
			    ProjectionOnExecutionInstance,
			    std::vector<std::vector<std::pair<signal_flow::Graph::vertex_descriptor, size_t>>>>
			    Projections;
			Projections projections;

			/**
			 * Translation of Population neurons to (multiple) atomic neurons in signal-flow graph.
			 * For each population (descriptor) a vector with equal ordering to the neurons of the
			 * population contains for each compartment on the logical neuron and for each atomic
			 * neuron on the logical neuron compartment a pair of signal-flow graph vertex
			 * descriptor and index of the corresponding atomic neuron on the signal-flow graph
			 * vertex property.
			 */
			typedef std::map<
			    PopulationOnExecutionInstance,
			    std::vector<std::map<
			        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
			        std::vector<std::pair<signal_flow::Graph::vertex_descriptor, size_t>>>>>
			    Populations;
			Populations populations;
		};

		std::map<grenade::common::ExecutionInstanceID, ExecutionInstance> execution_instances;
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
	    ProjectionOnNetwork descriptor, size_t index) const SYMBOL_VISIBLE;
	PlacedConnections get_placed_connections(ProjectionOnNetwork descriptor) const SYMBOL_VISIBLE;

	GENPYBIND_MANUAL({
		parent.def("__copy__", [](GENPYBIND_PARENT_TYPE const& self) {
			return grenade::vx::network::NetworkGraph(self);
		});
		parent.def(
		    "__deepcopy__",
		    [](GENPYBIND_PARENT_TYPE const& self, pybind11::dict) {
			    return grenade::vx::network::NetworkGraph(self);
		    },
		    pybind11::arg("memo"));
	})

private:
	std::shared_ptr<Network> m_network;

	signal_flow::Graph m_graph;
	GraphTranslation m_graph_translation;

	std::chrono::microseconds m_construction_duration;
	std::chrono::microseconds m_verification_duration;
	std::chrono::microseconds m_routing_duration;

	friend NetworkGraph build_network_graph(
	    std::shared_ptr<Network> const& network, RoutingResult const& routing_result);
	friend void update_network_graph(
	    NetworkGraph& network_graph, std::shared_ptr<Network> const& network);
	friend NetworkGraphStatistics extract_statistics(NetworkGraph const& network_graph);
};

} // namespace network
} // namespace grenade::vx
