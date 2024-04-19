#include "grenade/vx/network/abstract/multicompartment_neuron.h"

#include "grenade/common/graph_impl.tcc"

namespace grenade::common {
template class Graph<
    vx::network::abstract::Neuron,
    detail::UndirectedGraph,
    vx::network::abstract::Compartment,
    vx::network::abstract::CompartmentConnection,
    vx::network::abstract::CompartmentOnNeuron,
    vx::network::abstract::CompartmentConnectionOnNeuron,
    std::unique_ptr>;
} // namespace grenade::common

namespace grenade::vx::network::abstract {

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
std::pair<Neuron::CompartmentIterator, Neuron::CompartmentIterator> Neuron::compartment_iterators()
    const
{
	return this->vertices();
}

// Iterator over all CompartmentConnections
std::pair<Neuron::CompartmentConnectionIterator, Neuron::CompartmentConnectionIterator>
Neuron::compartment_connection_iterators() const
{
	return this->edges();
}

// Iterator to adjecent Compartments to given compartment
std::pair<Neuron::AdjacencyIterator, Neuron::AdjacencyIterator> Neuron::adjacent_compartments(
    CompartmentOnNeuron const& descriptor) const
{
	return this->adjacent_vertices(descriptor);
}

// Returns CompartmentID-Mappingt to other neuron if isomorphic
std::map<CompartmentOnNeuron, CompartmentOnNeuron> Neuron::isomorphism(Neuron const& other) const
{
	return Graph::isomorphism(other);
}

// Returns map of index to Compartment
std::map<CompartmentOnNeuron::Value, size_t> Neuron::get_compartment_index_map() const
{
	std::map<CompartmentOnNeuron::Value, size_t> mapping;
	size_t index = 0;
	for (auto compartment : boost::make_iterator_range(compartment_iterators())) {
		mapping.emplace(compartment.value(), index);
		index++;
	}
	return mapping;
}

// Checks if two compartments are neighbours
bool Neuron::neighbour(CompartmentOnNeuron const& source, CompartmentOnNeuron const& target) const
{
	for (auto compartment : boost::make_iterator_range(adjacent_compartments(source))) {
		if (compartment == target) {
			return true;
		}
	}
	return false;
}

// Checks if all compartments are connected
bool Neuron::compartments_connected() const
{
	return this->is_connected();
}

// Checks if Neuron Contains Compartment via Compartment-ID
bool Neuron::contains(CompartmentOnNeuron const& descriptor) const
{
	return Graph::contains(descriptor);
}

std::ostream& operator<<(std::ostream& os, Neuron const& neuron)
{
	os << "Neuron(\n";
	os << "\tCompartments: " << neuron.num_compartments() << "\n";
	for (auto compartment : boost::make_iterator_range(neuron.compartment_iterators())) {
		os << "\t\t" << compartment << "\n";
	}
	os << "\tConnections: " << neuron.num_compartment_connections() << "\n";
	;
	for (auto connection : boost::make_iterator_range(neuron.compartment_connection_iterators())) {
		os << "\t\t" << connection << "\n";
	}
	os << ")\n";

	return os;
}

} // namespace grenade::vx::network::abstract