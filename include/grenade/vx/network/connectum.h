#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/receptor.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "haldls/vx/v3/event.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <map>
#include <variant>
#include <vector>

namespace grenade::vx::network {

struct ConnectumConnection
{
	typedef std::tuple<
	    grenade::common::VertexOnTopology,
	    size_t,
	    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>
	    Source;
	Source source;

	halco::hicann_dls::vx::v3::AtomicNeuronOnDLS target;

	Receptor::Type receptor_type;

	grenade::common::VertexOnTopology projection;
	size_t connection_on_projection;

	bool operator==(ConnectumConnection const& other) const SYMBOL_VISIBLE;
	bool operator!=(ConnectumConnection const& other) const SYMBOL_VISIBLE;
	bool operator<(ConnectumConnection const& other) const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, ConnectumConnection const& config)
	    SYMBOL_VISIBLE;
};


std::vector<ConnectumConnection> generate_connectum_from_abstract_network(
    grenade::common::LinkedTopology const& topology) SYMBOL_VISIBLE;
std::vector<ConnectumConnection> generate_connectum_from_hardware_network(
    grenade::common::LinkedTopology const& topology) SYMBOL_VISIBLE;

} // namespace grenade::vx::network
