#include "grenade/vx/network/abstract/multicompartment_compartment_connection_on_neuron.h"

#include "grenade/vx/network/abstract/edge_on_graph_impl.tcc"

namespace grenade::vx::network {

template class EdgeOnGraph<CompartmentConnectionOnNeuron, detail::UndirectedGraph>;
} // namespace grenade::vx::network