#pragma once
#include "grenade/vx/network/abstract/multicompartment_neuron.h"

namespace grenade::vx::network::abstract {

// Returns a mapping of vertices if the graphs are isomorphic and the number of unmapped
template <typename Callback, typename CompartmentEquivalent>
void Neuron::isomorphism(
    Neuron const& other, Callback&& callback, CompartmentEquivalent&& compartment_equivalent) const
{
	Graph::isomorphism(other, callback, compartment_equivalent);
}


// Returns a mapping of vertices of the graphs largest subgraph and the number of unmapped
// vertices.
template <typename Callback, typename CompartmentEquivalent>
void Neuron::isomorphism_subgraph(
    Neuron const& other, Callback&& callback, CompartmentEquivalent&& compartment_equivalent) const
{
	Graph::isomorphism_subgraph(other, callback, compartment_equivalent);
}


} // namespace grenade::vx::network::abstract