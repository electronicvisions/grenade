#include "grenade/vx/network/abstract/multicompartment_compartment_on_neuron.h"

#include "grenade/common/vertex_on_graph_impl.tcc"

namespace grenade::common {

template class VertexOnGraph<vx::network::abstract::CompartmentOnNeuron, detail::UndirectedGraph>;

} // namespace grenade::common
