#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/network/population_on_execution_instance.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/network/projection_on_network.h"
#include "grenade/vx/signal_flow/graph.h"
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
	    PopulationOnExecutionInstance,
	    size_t,
	    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>
	    Source;
	Source source;

	halco::hicann_dls::vx::v3::AtomicNeuronOnDLS target;

	Receptor::Type receptor_type;
	lola::vx::v3::SynapseMatrix::Weight weight;

	ProjectionOnNetwork projection;
	size_t connection_on_projection;

	halco::hicann_dls::vx::v3::SynapseRowOnDLS synapse_row;
	halco::hicann_dls::vx::v3::SynapseOnSynapseRow synapse_on_row;

	bool operator==(ConnectumConnection const& other) const SYMBOL_VISIBLE;
	bool operator!=(ConnectumConnection const& other) const SYMBOL_VISIBLE;
	bool operator<(ConnectumConnection const& other) const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, ConnectumConnection const& config)
	    SYMBOL_VISIBLE;
};


struct Connectum
{
	std::map<grenade::common::ExecutionInstanceID, std::vector<ConnectumConnection>>
	    execution_instances;

	bool operator==(Connectum const& other) const SYMBOL_VISIBLE;
	bool operator!=(Connectum const& other) const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, Connectum const& connectum) SYMBOL_VISIBLE;
};

struct Network;
struct NetworkGraph;

Connectum generate_connectum_from_abstract_network(NetworkGraph const& network_graph)
    SYMBOL_VISIBLE;
Connectum generate_connectum_from_hardware_network(NetworkGraph const& network_graph)
    SYMBOL_VISIBLE;

} // namespace grenade::vx::network
