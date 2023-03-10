#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_logical/connection_routing_result.h"
#include "grenade/vx/network/placed_logical/network.h"
#include "grenade/vx/network/placed_logical/population.h"
#include "grenade/vx/network/placed_logical/projection.h"
#include "grenade/vx/network/placed_logical/routing/routing_constraints.h"
#include "grenade/vx/network/placed_logical/routing/source_on_padi_bus_manager.h"
#include "grenade/vx/network/placed_logical/routing/synapse_driver_on_dls_manager.h"
#include "grenade/vx/network/placed_logical/routing_options.h"
#include "grenade/vx/network/placed_logical/routing_result.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v3/event.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/padi.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synapse_driver.h"
#include "haldls/vx/v3/event.h"
#include "haldls/vx/v3/neuron.h"
#include "haldls/vx/v3/synapse_driver.h"
#include "hate/visibility.h"
#include "lola/vx/v3/synapse.h"
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>
#include <variant>
#include <vector>

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace grenade::vx::network::placed_logical::routing {

struct RoutingBuilder
{
	RoutingBuilder() SYMBOL_VISIBLE;

	typedef RoutingResult Result;

	Result route(
	    Network const& network,
	    ConnectionRoutingResult const& connection_routing_result,
	    std::optional<RoutingOptions> const& options = std::nullopt) const SYMBOL_VISIBLE;

private:
	void route_internal_crossbar(
	    SourceOnPADIBusManager::DisabledInternalRoutes& disabled_internal_routes,
	    RoutingConstraints const& constraints,
	    halco::common::typed_array<
	        RoutingConstraints::PADIBusConstraints,
	        halco::hicann_dls::vx::v3::PADIBusOnDLS>& padi_bus_constraints,
	    Result& result) const;

	std::pair<
	    std::vector<SourceOnPADIBusManager::InternalSource>,
	    std::vector<std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>>>
	get_internal_sources(
	    RoutingConstraints const& constraints,
	    halco::common::typed_array<
	        RoutingConstraints::PADIBusConstraints,
	        halco::hicann_dls::vx::v3::PADIBusOnDLS> const& padi_bus_constraints,
	    Network const& network) const;

	std::pair<
	    std::vector<SourceOnPADIBusManager::BackgroundSource>,
	    std::vector<std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>>>
	get_background_sources(
	    RoutingConstraints const& constraints,
	    halco::common::typed_array<
	        RoutingConstraints::PADIBusConstraints,
	        halco::hicann_dls::vx::v3::PADIBusOnDLS> const& padi_bus_constraints,
	    Network const& network) const;

	std::pair<
	    std::vector<SourceOnPADIBusManager::ExternalSource>,
	    std::vector<std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>>>
	get_external_sources(
	    RoutingConstraints const& constraints,
	    halco::common::typed_array<
	        RoutingConstraints::PADIBusConstraints,
	        halco::hicann_dls::vx::v3::PADIBusOnDLS> const& padi_bus_constraints,
	    Network const& network) const;

	std::map<
	    std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	    halco::hicann_dls::vx::v3::SpikeLabel>
	get_internal_labels(
	    std::vector<std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>> const& descriptors,
	    SourceOnPADIBusManager::Partition const& partition,
	    std::vector<SynapseDriverOnDLSManager::Allocation> const& allocations) const;

	std::map<
	    std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	    std::map<halco::hicann_dls::vx::v3::HemisphereOnDLS, halco::hicann_dls::vx::v3::SpikeLabel>>
	get_background_labels(
	    std::vector<std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>> const& descriptors,
	    std::vector<SourceOnPADIBusManager::BackgroundSource> const& background_sources,
	    SourceOnPADIBusManager::Partition const& partition,
	    std::vector<SynapseDriverOnDLSManager::Allocation> const& allocations) const;

	std::map<
	    std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	    std::map<halco::hicann_dls::vx::v3::PADIBusOnDLS, halco::hicann_dls::vx::v3::SpikeLabel>>
	get_external_labels(
	    std::vector<std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>> const& descriptors,
	    SourceOnPADIBusManager::Partition const& partition,
	    std::vector<SynapseDriverOnDLSManager::Allocation> const& allocations) const;

	void apply_source_labels(
	    RoutingConstraints const& constraints,
	    std::map<
	        std::tuple<
	            PopulationDescriptor,
	            size_t,
	            halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	        halco::hicann_dls::vx::v3::SpikeLabel> const& internal,
	    std::map<
	        std::tuple<
	            PopulationDescriptor,
	            size_t,
	            halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	        std::map<
	            halco::hicann_dls::vx::v3::HemisphereOnDLS,
	            halco::hicann_dls::vx::v3::SpikeLabel>> const& background,
	    std::map<
	        std::tuple<
	            PopulationDescriptor,
	            size_t,
	            halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	        std::map<
	            halco::hicann_dls::vx::v3::PADIBusOnDLS,
	            halco::hicann_dls::vx::v3::SpikeLabel>> const& external,
	    Network const& network,
	    Result& result) const;

	struct RoutedConnection
	{
		std::pair<ProjectionDescriptor, size_t> descriptor;
		halco::hicann_dls::vx::v3::AtomicNeuronOnDLS target;
	};

	struct PlacedConnection
	{
		halco::hicann_dls::vx::v3::SynapseRowOnDLS synapse_row;
		halco::hicann_dls::vx::v3::SynapseOnSynapseRow synapse_on_row;
	};

	std::map<std::pair<ProjectionDescriptor, size_t>, std::vector<PlacedConnection>>
	place_routed_connections(
	    std::vector<RoutedConnection> const& connections,
	    std::vector<halco::hicann_dls::vx::v3::SynapseRowOnDLS> const& synapse_rows) const;

	template <typename Connection>
	std::map<std::pair<ProjectionDescriptor, size_t>, std::vector<PlacedConnection>>
	place_routed_connections(
	    std::vector<Connection> const& connections,
	    std::map<Receptor::Type, std::vector<halco::hicann_dls::vx::v3::SynapseRowOnDLS>> const&
	        synapse_rows) const;

	template <typename Sources>
	std::map<std::pair<ProjectionDescriptor, size_t>, std::vector<PlacedConnection>>
	place_routed_connections(
	    std::vector<SourceOnPADIBusManager::Partition::Group> const& partition,
	    std::vector<std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>> const& descriptors,
	    Sources const& sources,
	    std::vector<SynapseDriverOnDLSManager::Allocation> const& padi_bus_allocations,
	    size_t offset,
	    RoutingConstraints const& constraints,
	    Network const& network,
	    Result& result) const;

	std::map<std::pair<ProjectionDescriptor, size_t>, std::vector<PlacedConnection>>
	place_routed_connections(
	    SourceOnPADIBusManager::Partition const& partition,
	    std::vector<std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>> const& internal_descriptors,
	    std::vector<std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>> const& background_descriptors,
	    std::vector<std::tuple<
	        PopulationDescriptor,
	        size_t,
	        halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>> const& external_descriptors,
	    std::vector<SourceOnPADIBusManager::InternalSource> const& internal_sources,
	    std::vector<SourceOnPADIBusManager::BackgroundSource> const& background_sources,
	    std::vector<SourceOnPADIBusManager::ExternalSource> const& external_sources,
	    std::vector<SynapseDriverOnDLSManager::Allocation> const& padi_bus_allocations,
	    RoutingConstraints const& constraints,
	    Network const& network,
	    Result& result) const;

	void apply_routed_connections(
	    std::map<std::pair<ProjectionDescriptor, size_t>, std::vector<PlacedConnection>> const&
	        placed_connections,
	    std::map<
	        std::tuple<
	            PopulationDescriptor,
	            size_t,
	            halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	        halco::hicann_dls::vx::v3::SpikeLabel> const& internal_labels,
	    std::map<
	        std::tuple<
	            PopulationDescriptor,
	            size_t,
	            halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	        std::map<
	            halco::hicann_dls::vx::v3::HemisphereOnDLS,
	            halco::hicann_dls::vx::v3::SpikeLabel>> const& background_labels,
	    std::map<
	        std::tuple<
	            PopulationDescriptor,
	            size_t,
	            halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>,
	        std::map<
	            halco::hicann_dls::vx::v3::PADIBusOnDLS,
	            halco::hicann_dls::vx::v3::SpikeLabel>> const& external_labels,
	    Network const& network,
	    Result& result) const;

	void apply_crossbar_nodes_from_l2(Result& result) const;
	void apply_crossbar_nodes_from_background(Result& result) const;
	void apply_crossbar_nodes_internal(Result& result) const;
	void apply_crossbar_nodes_from_internal_to_l2(Result& result) const;

	log4cxx::LoggerPtr m_logger;
};

} // namespace grenade::vx::network::placed_logical::routing
