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

bool Neuron::valid()
{
	for (auto compartment : boost::make_iterator_range(compartment_iterators())) {
		if (!(get(compartment).valid())) {
			return false;
		}
	}
	return true;
}

CompartmentOnNeuron Neuron::add_compartment(Compartment const& compartment)
{
	return this->add_vertex(compartment);
}
void Neuron::remove_compartment(CompartmentOnNeuron descriptor)
{
	this->remove_vertex(descriptor);
}

CompartmentConnectionOnNeuron Neuron::add_compartment_connection(
    CompartmentOnNeuron source, CompartmentOnNeuron target, CompartmentConnection const& edge)
{
	return this->add_edge(source, target, edge);
}
void Neuron::remove_compartment_connection(CompartmentConnectionOnNeuron descriptor)
{
	this->remove_edge(descriptor);
}

Compartment const& Neuron::get(CompartmentOnNeuron const& descriptor) const
{
	return Graph::get(descriptor);
}
void Neuron::set(CompartmentOnNeuron const& descriptor, Compartment const& compartment)
{
	Graph::set(descriptor, compartment);
}

CompartmentConnection const& Neuron::get(CompartmentConnectionOnNeuron const& descriptor) const
{
	return Graph::get(descriptor);
}
void Neuron::set(
    CompartmentConnectionOnNeuron const& descriptor, CompartmentConnection const& connection)
{
	Graph::set(descriptor, connection);
}

void Neuron::clear()
{
	Graph::clear();
}

size_t Neuron::num_compartments() const
{
	return this->num_vertices();
}
size_t Neuron::num_compartment_connections() const
{
	return this->num_edges();
}

int Neuron::in_degree(CompartmentOnNeuron const& descriptor) const
{
	return Graph::in_degree(descriptor);
}
int Neuron::out_degree(CompartmentOnNeuron const& descriptor) const
{
	return Graph::out_degree(descriptor);
}

CompartmentOnNeuron Neuron::source(CompartmentConnectionOnNeuron const& descriptor) const
{
	return Graph::source(descriptor);
}
CompartmentOnNeuron Neuron::target(CompartmentConnectionOnNeuron const& descriptor) const
{
	return Graph::target(descriptor);
}

std::pair<Neuron::CompartmentIterator, Neuron::CompartmentIterator> Neuron::compartment_iterators()
    const
{
	return this->vertices();
}

std::pair<Neuron::CompartmentConnectionIterator, Neuron::CompartmentConnectionIterator>
Neuron::compartment_connection_iterators() const
{
	return this->edges();
}

std::pair<Neuron::AdjacencyIterator, Neuron::AdjacencyIterator> Neuron::adjacent_compartments(
    CompartmentOnNeuron const& descriptor) const
{
	return this->adjacent_vertices(descriptor);
}

std::map<CompartmentOnNeuron, CompartmentOnNeuron> Neuron::isomorphism(Neuron const& other) const
{
	return Graph::isomorphism(other);
}

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

bool Neuron::neighbour(CompartmentOnNeuron const& source, CompartmentOnNeuron const& target) const
{
	for (auto compartment : boost::make_iterator_range(adjacent_compartments(source))) {
		if (compartment == target) {
			return true;
		}
	}
	return false;
}

bool Neuron::compartments_connected() const
{
	return this->is_connected();
}

bool Neuron::contains(CompartmentOnNeuron const& descriptor) const
{
	return Graph::contains(descriptor);
}

void Neuron::write_graphviz(std::ostream& file, std::string name)
{
	file << "graph " << name << " {\n";
	for (auto connection : boost::make_iterator_range(compartment_connection_iterators())) {
		auto compartment_a = source(connection);
		auto compartment_b = target(connection);
		file << compartment_a << "--" << compartment_b << "\n";
	}
	file << "}\n";
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