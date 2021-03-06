#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/network/madc_recording.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/routing_result.h"
#include "halco/hicann-dls/vx/v2/neuron.h"
#include "halco/hicann-dls/vx/v2/padi.h"
#include "halco/hicann-dls/vx/v2/synapse.h"
#include "halco/hicann-dls/vx/v2/synapse_driver.h"
#include "hate/visibility.h"
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace log4cxx {
class Logger;
} // namespace log4cxx

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

/**
 * Build a hardware graph representation for a given network with the given routing result.
 * @param network Network for which to build hardware graph representation
 * @param routing_result Routing result to use matching the given network
 */
NetworkGraph GENPYBIND(visible) build_network_graph(
    std::shared_ptr<Network> const& network, RoutingResult const& routing_result) SYMBOL_VISIBLE;


/**
 * Network graph builder wrapper for network.
 */
class NetworkGraphBuilder
{
public:
	NetworkGraphBuilder(Network const& network) SYMBOL_VISIBLE;

	struct Resources
	{
		struct PlacedPopulation
		{
			std::map<halco::hicann_dls::vx::HemisphereOnDLS, Graph::vertex_descriptor> neurons;
		};

		struct PlacedProjection
		{
			std::map<halco::hicann_dls::vx::HemisphereOnDLS, Graph::vertex_descriptor> synapses;
		};

		std::optional<Graph::vertex_descriptor> external_input;
		std::optional<Graph::vertex_descriptor> crossbar_l2_input;
		std::map<halco::hicann_dls::vx::v2::CrossbarNodeOnDLS, Graph::vertex_descriptor>
		    crossbar_nodes;
		std::map<halco::hicann_dls::vx::v2::PADIBusOnDLS, Graph::vertex_descriptor> padi_busses;
		std::map<halco::hicann_dls::vx::v2::SynapseDriverOnDLS, Graph::vertex_descriptor>
		    synapse_drivers;
		std::map<halco::hicann_dls::vx::v2::NeuronEventOutputOnDLS, Graph::vertex_descriptor>
		    neuron_event_outputs;
		std::map<PopulationDescriptor, PlacedPopulation> populations;
		std::map<ProjectionDescriptor, PlacedProjection> projections;
		std::optional<Graph::vertex_descriptor> external_output;
		std::optional<Graph::vertex_descriptor> madc_output;
	};

	static std::vector<Input> get_inputs(Graph const& graph, Graph::vertex_descriptor descriptor);

	void add_external_input(
	    Graph& graph, Resources& resources, coordinate::ExecutionInstance const& instance) const;

	void add_padi_bus(
	    Graph& graph,
	    Resources& resources,
	    halco::hicann_dls::vx::PADIBusOnDLS const& coordinate,
	    coordinate::ExecutionInstance const& instance) const;

	void add_crossbar_node(
	    Graph& graph,
	    Resources& resources,
	    halco::hicann_dls::vx::CrossbarNodeOnDLS const& coordinate,
	    RoutingResult const& connection_result,
	    coordinate::ExecutionInstance const& instance) const;

	void add_synapse_driver(
	    Graph& graph,
	    Resources& resources,
	    halco::hicann_dls::vx::SynapseDriverOnDLS const& coordinate,
	    RoutingResult const& connection_result,
	    coordinate::ExecutionInstance const& instance) const;

	void add_neuron_event_output(
	    Graph& graph,
	    Resources& resources,
	    halco::hicann_dls::vx::NeuronEventOutputOnDLS const& coordinate,
	    coordinate::ExecutionInstance const& instance) const;

	void add_synapse_array_view_sparse(
	    Graph& graph,
	    Resources& resources,
	    ProjectionDescriptor descriptor,
	    RoutingResult const& connection_result,
	    coordinate::ExecutionInstance const& instance) const;

	void add_population(
	    Graph& graph,
	    Resources& resources,
	    std::map<halco::hicann_dls::vx::v2::HemisphereOnDLS, Input> const& input,
	    PopulationDescriptor const& descriptor,
	    RoutingResult const& connection_result,
	    coordinate::ExecutionInstance const& instance) const;

	void add_projection_from_external_input(
	    Graph& graph,
	    Resources& resources,
	    ProjectionDescriptor const& descriptor,
	    RoutingResult const& connection_result,
	    coordinate::ExecutionInstance const& instance) const;

	void add_projection_from_internal_input(
	    Graph& graph,
	    Resources& resources,
	    ProjectionDescriptor const& descriptor,
	    RoutingResult const& connection_result,
	    coordinate::ExecutionInstance const& instance) const;

	void add_populations(
	    Graph& graph,
	    Resources& resources,
	    RoutingResult const& connection_result,
	    coordinate::ExecutionInstance const& instance) const;

	void add_neuron_event_outputs(
	    Graph& graph, Resources& resources, coordinate::ExecutionInstance const& instance) const;

	void add_external_output(
	    Graph& graph,
	    Resources& resources,
	    RoutingResult const& connection_result,
	    coordinate::ExecutionInstance const& instance) const;

	void add_madc_recording(
	    Graph& graph,
	    Resources& resources,
	    MADCRecording const& madc_recording,
	    coordinate::ExecutionInstance const& instance) const;

	NetworkGraph::SpikeLabels get_spike_labels(RoutingResult const& connection_result);

private:
	Network const& m_network;
	log4cxx::Logger* m_logger;
};

} // namespace network

} // namespace grenade::vx
