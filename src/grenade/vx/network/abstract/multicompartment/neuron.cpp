#include "grenade/vx/network/abstract/multicompartment/neuron.h"
#include "grenade/common/graph_impl.tcc"
#include <fstream>

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

bool Neuron::ParameterSpace::valid(Parameterization const& parameterization) const
{
	for (auto [compartment, compartment_parameterization] : parameterization.compartments) {
		if (!compartments.at(compartment).valid(compartment_parameterization)) {
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

size_t Neuron::get_compartment_degree(CompartmentOnNeuron const& descriptor) const
{
	assert(Graph::out_degree(descriptor) == Graph::in_degree(descriptor));
	return Graph::out_degree(descriptor);
}

CompartmentOnNeuron Neuron::get_max_degree_compartment() const
{
	CompartmentOnNeuron compartment_max_degree = *(compartments().first);
	for (auto compartment : boost::make_iterator_range(compartments())) {
		if (get_compartment_degree(compartment) > get_compartment_degree(compartment_max_degree)) {
			compartment_max_degree = compartment;
		}
	}
	return compartment_max_degree;
}

CompartmentOnNeuron Neuron::source(CompartmentConnectionOnNeuron const& descriptor) const
{
	return Graph::source(descriptor);
}

CompartmentOnNeuron Neuron::target(CompartmentConnectionOnNeuron const& descriptor) const
{
	return Graph::target(descriptor);
}

// Iterators over Compartments
std::pair<Neuron::CompartmentIterator, Neuron::CompartmentIterator> Neuron::compartments() const
{
	return this->vertices();
}

std::pair<Neuron::CompartmentConnectionIterator, Neuron::CompartmentConnectionIterator>
Neuron::compartment_connections() const
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
	for (auto compartment : boost::make_iterator_range(compartments())) {
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


size_t Neuron::branch_size(
    CompartmentOnNeuron const& compartment,
    std::set<CompartmentOnNeuron>& marked_compartments) const
{
	size_t size = 1;
	marked_compartments.emplace(compartment);

	for (auto adjacent_compartment :
	     boost::make_iterator_range(adjacent_compartments(compartment))) {
		if (marked_compartments.contains(adjacent_compartment)) {
			continue;
		}
		size += branch_size(adjacent_compartment, marked_compartments);
	}
	return size;
}

bool Neuron::is_chain(
    CompartmentOnNeuron const& compartment,
    std::set<CompartmentOnNeuron>& marked_compartments) const
{
	if (get_compartment_degree(compartment) > 2) {
		return false;
	}

	marked_compartments.emplace(compartment);

	for (auto adjacent_compartment :
	     boost::make_iterator_range(adjacent_compartments(compartment))) {
		if (marked_compartments.contains(adjacent_compartment)) {
			continue;
		}
		if (!is_chain(adjacent_compartment, marked_compartments)) {
			return false;
		}
	}
	return true;
}

std::vector<CompartmentOnNeuron> Neuron::branch_compartments(
    CompartmentOnNeuron const& compartment, CompartmentOnNeuron const& blacklist_compartment) const
{
	std::vector<CompartmentOnNeuron> branch;
	branch.push_back(compartment);

	std::set<CompartmentOnNeuron> marked_compartments;
	marked_compartments.emplace(compartment);
	marked_compartments.emplace(blacklist_compartment);

	std::stack<CompartmentOnNeuron> compartment_queue;
	compartment_queue.push(compartment);

	while (!compartment_queue.empty()) {
		auto current_compartment = compartment_queue.top();
		compartment_queue.pop();

		for (auto adjacent_compartment :
		     boost::make_iterator_range(adjacent_compartments(current_compartment))) {
			if (!marked_compartments.contains(adjacent_compartment)) {
				branch.push_back(adjacent_compartment);
				marked_compartments.emplace(adjacent_compartment);

				compartment_queue.push(adjacent_compartment);
			}
		}

		if (branch.size() > num_compartments()) {
			throw std::logic_error("Chain is looped.");
		}
	}
	return branch;
}

CompartmentNeighbours Neuron::classify_neighbours(
    CompartmentOnNeuron const& compartment,
    std::set<CompartmentOnNeuron> neighbours_whitelist) const
{
	CompartmentNeighbours neighbours;

	std::set<CompartmentOnNeuron> marked_compartments;
	marked_compartments.emplace(compartment);

	for (auto adjacent_compartment :
	     boost::make_iterator_range(adjacent_compartments(compartment))) {
		if (neighbours_whitelist.size() > 0 &&
		    !neighbours_whitelist.contains(adjacent_compartment)) {
			continue;
		}
		// Leaf
		if (get_compartment_degree(adjacent_compartment) == 1) {
			neighbours.leafs.push_back(adjacent_compartment);
		}
		// Chain
		else if (is_chain(adjacent_compartment, marked_compartments)) {
			neighbours.chains.push_back(adjacent_compartment);
		}
		// Branch
		else {
			neighbours.branches.push_back(adjacent_compartment);
		}
	}

	return neighbours;
}

bool Neuron::compartments_connected() const
{
	return this->is_connected();
}

bool Neuron::contains(CompartmentOnNeuron const& descriptor) const
{
	return Graph::contains(descriptor);
}

bool Neuron::valid(ParameterSpace const& parameter_space) const
{
	for (auto& [compartment, compartment_parameter_space] : parameter_space.compartments) {
		if (!get(compartment).valid(compartment_parameter_space)) {
			return false;
		}
	}
	return true;
}

// Writes neuron topology in graphviz format to be plotted later
void Neuron::write_graphviz(std::string filename, std::string name)
{
	std::ofstream file;
	file.open(filename);
	file << "graph " << name << " {\n";
	for (auto connection : boost::make_iterator_range(compartment_connections())) {
		auto compartment_a = source(connection);
		auto compartment_b = target(connection);
		file << compartment_a << "--" << compartment_b << "\n";
	}
	file << "}\n";
	file.close();
}

std::ostream& operator<<(std::ostream& os, Neuron const& neuron)
{
	os << "Neuron(\n";
	os << "\tCompartments: " << neuron.num_compartments() << "\n";
	for (auto compartment : boost::make_iterator_range(neuron.compartments())) {
		os << "\t\t" << compartment << "\n";
	}
	os << "\tConnections: " << neuron.num_compartment_connections() << "\n";
	;
	for (auto connection : boost::make_iterator_range(neuron.compartment_connections())) {
		os << "\t\t" << connection << "\n";
	}
	os << ")\n";

	return os;
}

} // namespace grenade::vx::network::abstract