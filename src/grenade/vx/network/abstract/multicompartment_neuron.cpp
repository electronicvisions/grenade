#include "grenade/vx/network/abstract/multicompartment_neuron.h"

#include "grenade/common/graph_impl.tcc"

namespace grenade::common {
template class Graph<
    vx::network::Neuron,
    detail::UndirectedGraph,
    vx::network::Compartment,
    vx::network::CompartmentConnection,
    vx::network::CompartmentOnNeuron,
    vx::network::CompartmentConnectionOnNeuron,
    std::unique_ptr>;
} // namespace grenade::common

namespace grenade::vx::network {

// Check Neuron Structur for Rulebreaks
bool Neuron::valid()
{
	for (auto compartment : boost::make_iterator_range(compartment_iterators())) {
		if (!(get(compartment).valid())) {
			return false;
		}
	}
	return true;
}

// Add and Remove Compartment
CompartmentOnNeuron Neuron::add_compartment(Compartment const& compartment)
{
	return this->add_vertex(compartment);
}
void Neuron::remove_compartment(CompartmentOnNeuron descriptor)
{
	this->remove_vertex(descriptor);
}

// Add and Remove CompartmentConnection
CompartmentConnectionOnNeuron Neuron::add_compartment_connection(
    CompartmentOnNeuron source, CompartmentOnNeuron target, CompartmentConnection const& edge)
{
	return this->add_edge(source, target, edge);
}
void Neuron::remove_compartment_connection(CompartmentConnectionOnNeuron descriptor)
{
	this->remove_edge(descriptor);
}

// Get and Set Compartment via Compartment-ID
Compartment const& Neuron::get(CompartmentOnNeuron const& descriptor) const
{
	return Graph::get(descriptor);
}
void Neuron::set(CompartmentOnNeuron const& descriptor, Compartment const& compartment)
{
	Graph::set(descriptor, compartment);
}

// Get and Set CompartmentConnection via CompartmentConnection-ID
CompartmentConnection const& Neuron::get(CompartmentConnectionOnNeuron const& descriptor) const
{
	return Graph::get(descriptor);
}
void Neuron::set(
    CompartmentConnectionOnNeuron const& descriptor, CompartmentConnection const& connection)
{
	Graph::set(descriptor, connection);
}

// Number of Compartments and CompartmentConnections
size_t Neuron::num_compartments() const
{
	return this->num_vertices();
}
size_t Neuron::num_compartment_connections() const
{
	return this->num_edges();
}

// Number of in and out-going Connections of Compartment
int Neuron::in_degree(CompartmentOnNeuron const& descriptor) const
{
	return Graph::in_degree(descriptor);
}
int Neuron::out_degree(CompartmentOnNeuron const& descriptor) const
{
	return Graph::out_degree(descriptor);
}

// Source and Target of CompartmentConnection via CompartmentConnection-ID
CompartmentOnNeuron Neuron::source(CompartmentConnectionOnNeuron const& descriptor) const
{
	return Graph::source(descriptor);
}
CompartmentOnNeuron Neuron::target(CompartmentConnectionOnNeuron const& descriptor) const
{
	return Graph::target(descriptor);
}

// Iterators over Compartments
typedef common::Graph<
    Neuron,
    common::detail::UndirectedGraph,
    Compartment,
    CompartmentConnection,
    CompartmentOnNeuron,
    CompartmentConnectionOnNeuron,
    std::unique_ptr>::VertexIterator CompartmentIterator;
std::pair<CompartmentIterator, CompartmentIterator> Neuron::compartment_iterators() const
{
	return this->vertices();
}


// Checks if Neuron Contains Compartment via Compartment-ID
bool Neuron::contains(CompartmentOnNeuron const& descriptor) const
{
	return Graph::contains(descriptor);
}
} // namespace grenade::vx::network