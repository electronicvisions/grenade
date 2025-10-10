#pragma once
#include "grenade/vx/network/abstract/multicompartment_unplaced_neuron_circuit.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) NeuronCircuit : public UnplacedNeuronCircuit
{
	std::optional<CompartmentOnNeuron> compartment;

	NeuronCircuit() = default;
};

} // namespace abstract
} // namespace grenade::vx::network