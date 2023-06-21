#pragma once
#include "grenade/vx/network/population_descriptor.h"
#include "grenade/vx/network/projection.h"
#include "grenade/vx/signal_flow/graph.h"
#include "haldls/vx/v3/event.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <variant>
#include <vector>

namespace grenade::vx::network {

struct ConnectumConnection
{
	typedef std::
	    tuple<PopulationDescriptor, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron>
	        Source;
	Source source;

	halco::hicann_dls::vx::v3::AtomicNeuronOnDLS target;

	Receptor::Type receptor_type;
	lola::vx::v3::SynapseMatrix::Weight weight;

	bool operator==(ConnectumConnection const& other) const SYMBOL_VISIBLE;
	bool operator!=(ConnectumConnection const& other) const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, ConnectumConnection const& config)
	    SYMBOL_VISIBLE;
};

typedef std::vector<ConnectumConnection> Connectum;

struct Network;
struct NetworkGraph;

Connectum generate_connectum_from_abstract_network(NetworkGraph const& network_graph)
    SYMBOL_VISIBLE;
Connectum generate_connectum_from_hardware_network(NetworkGraph const& network_graph)
    SYMBOL_VISIBLE;

} // namespace grenade::vx::network
