#include "grenade/vx/network/abstract/multicompartment_compartment_on_neuron.h"

#include "grenade/vx/network/abstract/vertex_on_graph_impl.tcc"

namespace grenade::vx::network {

template class VertexOnGraph<CompartmentOnNeuron, detail::UndirectedGraph>;

} // namespace grenade::vx::network