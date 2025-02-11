#include "grenade/vx/network/abstract/multicompartment_compartment_connection_on_neuron.h"

#include "grenade/common/edge_on_graph_impl.tcc"

namespace grenade::common {
template class EdgeOnGraph<
    vx::network::abstract::CompartmentConnectionOnNeuron,
    detail::UndirectedGraph>;
} // namespace grenade::common
