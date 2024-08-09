#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/population_on_network.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/visibility.h"
#include <cstddef>
#include <iosfwd>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct AtomicNeuronOnExecutionInstance;


struct GENPYBIND(visible) AtomicNeuronOnNetwork
{
	PopulationOnNetwork population{};
	size_t neuron_on_population{0};
	halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron compartment_on_neuron{};
	size_t atomic_neuron_on_compartment{0};

	AtomicNeuronOnNetwork() = default;
	AtomicNeuronOnNetwork(
	    AtomicNeuronOnExecutionInstance const& atomic_neuron_on_execution_instance,
	    grenade::common::ExecutionInstanceID const& execution_instance) SYMBOL_VISIBLE;

	AtomicNeuronOnNetwork(
	    PopulationOnNetwork population,
	    size_t neuron_on_population,
	    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron compartment_on_neuron,
	    size_t atomic_neuron_on_compartment) SYMBOL_VISIBLE;

	bool operator==(AtomicNeuronOnNetwork const& other) const SYMBOL_VISIBLE;
	bool operator!=(AtomicNeuronOnNetwork const& other) const SYMBOL_VISIBLE;
	bool operator<=(AtomicNeuronOnNetwork const& other) const SYMBOL_VISIBLE;
	bool operator>=(AtomicNeuronOnNetwork const& other) const SYMBOL_VISIBLE;
	bool operator<(AtomicNeuronOnNetwork const& other) const SYMBOL_VISIBLE;
	bool operator>(AtomicNeuronOnNetwork const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, AtomicNeuronOnNetwork const& value)
	    SYMBOL_VISIBLE;
};

} // namespace network
} // namespace grenade::vx
