#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/cadc_recording.h"
#include "grenade/vx/network/madc_recording.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/plasticity_rule.h"
#include "grenade/vx/network/population_on_network.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/routing_result.h"
#include "grenade/vx/signal_flow/graph.h"
#include "halco/hicann-dls/vx/v3/background.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/padi.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synapse_driver.h"
#include "hate/visibility.h"
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Build a hardware network representation for a given network.
 * @param network Network for which to build hardware network representation
 * @param routing_result Routing result to use to build hardware network representation
 */
NetworkGraph GENPYBIND(visible) build_network_graph(
    std::shared_ptr<Network> const& network, RoutingResult const& routing_result) SYMBOL_VISIBLE;


/**
 * Update an exisiting hardware graph representation.
 * For this to work, no new routing has to have been required.
 * @param network_graph Existing hardware graph representation to update or fill with newly built
 * instance
 * @param network New network for which to update or build
 */
void GENPYBIND(visible) update_network_graph(
    NetworkGraph& network_graph, std::shared_ptr<Network> const& network) SYMBOL_VISIBLE;

/**
 * Network graph builder wrapper for network.
 */
class NetworkGraphBuilder
{
public:
	NetworkGraphBuilder(Network const& network) SYMBOL_VISIBLE;

	struct Resources
	{
		struct ExecutionInstance
		{
			struct PlacedPopulation
			{
				std::map<
				    halco::hicann_dls::vx::HemisphereOnDLS,
				    signal_flow::Graph::vertex_descriptor>
				    neurons;
			};

			struct PlacedProjection
			{
				std::map<
				    halco::hicann_dls::vx::HemisphereOnDLS,
				    signal_flow::Graph::vertex_descriptor>
				    synapses;
			};

			std::optional<signal_flow::Graph::vertex_descriptor> crossbar_l2_input;
			std::map<
			    halco::hicann_dls::vx::v3::CrossbarNodeOnDLS,
			    signal_flow::Graph::vertex_descriptor>
			    crossbar_nodes;
			std::map<halco::hicann_dls::vx::v3::PADIBusOnDLS, signal_flow::Graph::vertex_descriptor>
			    padi_busses;
			std::map<
			    halco::hicann_dls::vx::v3::SynapseDriverOnDLS,
			    signal_flow::Graph::vertex_descriptor>
			    synapse_drivers;
			std::map<
			    halco::hicann_dls::vx::v3::NeuronEventOutputOnDLS,
			    signal_flow::Graph::vertex_descriptor>
			    neuron_event_outputs;
			std::map<PopulationOnExecutionInstance, PlacedPopulation> populations;
			std::map<ProjectionOnExecutionInstance, PlacedProjection> projections;
			NetworkGraph::GraphTranslation::ExecutionInstance graph_translation;
		};

		std::map<grenade::common::ExecutionInstanceID, ExecutionInstance> execution_instances;
	};

	static std::vector<signal_flow::Input> get_inputs(
	    signal_flow::Graph const& graph, signal_flow::Graph::vertex_descriptor descriptor);

	void add_external_input(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_background_spike_sources(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    grenade::common::ExecutionInstanceID const& instance,
	    RoutingResult const& routing_result) const;

	void add_padi_bus(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    halco::hicann_dls::vx::PADIBusOnDLS const& coordinate,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_crossbar_node(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    halco::hicann_dls::vx::CrossbarNodeOnDLS const& coordinate,
	    RoutingResult const& connection_result,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_synapse_driver(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    halco::hicann_dls::vx::SynapseDriverOnDLS const& coordinate,
	    RoutingResult const& connection_result,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_neuron_event_output(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    halco::hicann_dls::vx::NeuronEventOutputOnDLS const& coordinate,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_synapse_array_view_sparse(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    ProjectionOnExecutionInstance descriptor,
	    RoutingResult const& connection_result,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_population(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    std::map<halco::hicann_dls::vx::v3::HemisphereOnDLS, std::vector<signal_flow::Input>> const&
	        input,
	    PopulationOnExecutionInstance const& descriptor,
	    RoutingResult const& connection_result,
	    grenade::common::ExecutionInstanceID const& instance) const;

	std::map<halco::hicann_dls::vx::v3::HemisphereOnDLS, signal_flow::Input>
	add_projection_from_external_input(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    ProjectionOnExecutionInstance const& descriptor,
	    RoutingResult const& connection_result,
	    grenade::common::ExecutionInstanceID const& instance) const;

	std::map<halco::hicann_dls::vx::v3::HemisphereOnDLS, signal_flow::Input>
	add_projection_from_background_spike_source(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    ProjectionOnExecutionInstance const& descriptor,
	    RoutingResult const& connection_result,
	    grenade::common::ExecutionInstanceID const& instance) const;

	std::map<halco::hicann_dls::vx::v3::HemisphereOnDLS, signal_flow::Input>
	add_projection_from_internal_input(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    ProjectionOnExecutionInstance const& descriptor,
	    RoutingResult const& connection_result,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_populations(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    RoutingResult const& connection_result,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_neuron_event_outputs(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_external_output(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    RoutingResult const& connection_result,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_madc_recording(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    MADCRecording const& madc_recording,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_cadc_recording(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    CADCRecording const& cadc_recording,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_pad_recording(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    PadRecording const& cadc_recording,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void add_plasticity_rules(
	    signal_flow::Graph& graph,
	    Resources& resources,
	    grenade::common::ExecutionInstanceID const& instance) const;

	void calculate_spike_labels(
	    Resources& resources,
	    RoutingResult const& connection_result,
	    grenade::common::ExecutionInstanceID const& instance) const;

	common::EntityOnChip::ChipCoordinate get_chip_coordinate(
	    grenade::common::ExecutionInstanceID const& instance) const;

private:
	Network const& m_network;
	log4cxx::LoggerPtr m_logger;
};

} // namespace network
} // namespace grenade::vx
