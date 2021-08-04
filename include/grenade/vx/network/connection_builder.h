#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/routing_result.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v2/neuron.h"
#include "halco/hicann-dls/vx/v2/padi.h"
#include "halco/hicann-dls/vx/v2/synapse.h"
#include "halco/hicann-dls/vx/v2/synapse_driver.h"
#include "haldls/vx/v2/event.h"
#include "haldls/vx/v2/neuron.h"
#include "haldls/vx/v2/synapse_driver.h"
#include "hate/visibility.h"
#include "lola/vx/v2/synapse.h"
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>
#include <variant>
#include <vector>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

/**
 * Get whether the current network requires routing compared to the old network.
 * @param current Current network
 * @param old Old network
 * @return Boolean value
 */
bool GENPYBIND(visible) requires_routing(
    std::shared_ptr<Network> const& current, std::shared_ptr<Network> const& old) SYMBOL_VISIBLE;

/**
 * Route given network.
 * @param network Placed but not routed network to use
 * @return Routing result containing placement and label information for given network
 */
RoutingResult GENPYBIND(visible)
    build_routing(std::shared_ptr<Network> const& network) SYMBOL_VISIBLE;

struct ConnectionBuilder
{
	ConnectionBuilder() SYMBOL_VISIBLE;

	typedef RoutingResult Result;

	struct UsedSynapseRow
	{
		halco::hicann_dls::vx::v2::SynapseDriverOnDLS synapse_driver;
		halco::hicann_dls::vx::v2::SynapseRowOnSynapseDriver synapse_row;
		std::set<halco::hicann_dls::vx::v2::AtomicNeuronOnDLS> neurons_post;
		Projection::ReceptorType receptor_type;
	};

	struct UsedExternalSynapseDriver
	{
		std::vector<std::pair<PopulationDescriptor, size_t>> pres;

		struct Row
		{
			std::set<halco::hicann_dls::vx::v2::NeuronColumnOnDLS> neurons_post;
			Projection::ReceptorType receptor_type;
		};
		std::map<halco::hicann_dls::vx::v2::SynapseRowOnSynapseDriver, Row> rows;
	};

	UsedSynapseRow const& get_synapse_row(
	    halco::hicann_dls::vx::v2::AtomicNeuronOnDLS pre,
	    halco::hicann_dls::vx::v2::AtomicNeuronOnDLS post,
	    Projection::ReceptorType receptor_type);

	halco::hicann_dls::vx::v2::SynapseDriverOnSynapseDriverBlock get_synapse_driver(
	    std::pair<PopulationDescriptor, size_t> const& pre,
	    std::set<halco::hicann_dls::vx::v2::NeuronColumnOnDLS> const& post,
	    halco::hicann_dls::vx::v2::HemisphereOnDLS const& hemisphere,
	    Projection::ReceptorType receptor_type);

	Result::PlacedConnection get_placed_connection(
	    halco::hicann_dls::vx::v2::AtomicNeuronOnDLS pre,
	    halco::hicann_dls::vx::v2::AtomicNeuronOnDLS post,
	    UsedSynapseRow const& synapse_row,
	    Projection::Connection const& connection) const;

	Result::PlacedConnection get_placed_connection(
	    std::pair<PopulationDescriptor, size_t> pre,
	    halco::hicann_dls::vx::v2::AtomicNeuronOnDLS post,
	    halco::hicann_dls::vx::v2::SynapseDriverOnSynapseDriverBlock const& synapse_driver,
	    Projection::Connection const& connection) const;

	std::vector<UsedSynapseRow> const& get_used_synapse_rows() const;

	void calculate_free_for_external_synapse_drivers();

	typedef halco::common::typed_array<
	    std::deque<halco::hicann_dls::vx::v2::SynapseDriverOnSynapseDriverBlock>,
	    halco::hicann_dls::vx::v2::HemisphereOnDLS>
	    FreeForExternalSynapseDrivers;

	std::vector<UsedSynapseRow> m_used_synapse_rows_internal{};
	halco::common::typed_array<size_t, halco::hicann_dls::vx::v2::PADIBusOnDLS> m_used_padi_rows;
	FreeForExternalSynapseDrivers m_free_for_external_synapse_drivers;
	std::map<
	    halco::hicann_dls::vx::v2::HemisphereOnDLS,
	    std::map<
	        halco::hicann_dls::vx::v2::SynapseDriverOnSynapseDriverBlock,
	        UsedExternalSynapseDriver>>
	    m_used_synapse_drivers_external{};

	std::map<
	    halco::hicann_dls::vx::v2::SynapseDriverOnDLS,
	    haldls::vx::v2::SynapseDriverConfig::RowAddressCompareMask>
	    m_synapse_driver_compare_mask;
};

} // namespace network

} // namespace grenade::vx
