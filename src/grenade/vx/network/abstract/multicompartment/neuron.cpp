#include "grenade/vx/network/abstract/multicompartment/neuron.h"

#include "grenade/common/graph_impl.tcc"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/population.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/multi_index_sequence_dimension_unit/mechanism_on_compartment.h"
#include "hate/indent.h"
#include "hate/join.h"
#include <fstream>
#include <log4cxx/logger.h>

namespace grenade::common {
template class Graph<
    vx::network::abstract::Neuron,
    detail::UndirectedGraph,
    vx::network::abstract::Compartment,
    vx::network::abstract::CompartmentConnection,
    CompartmentOnNeuron,
    vx::network::abstract::CompartmentConnectionOnNeuron,
    std::unique_ptr>;
} // namespace grenade::common

namespace grenade::vx::network::abstract {

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace::Parameterization>
Neuron::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	Parameterization ret;
	for (auto const& [compartment_on_neuron, compartment] : compartments) {
		ret.compartments.emplace(compartment_on_neuron, compartment.get_section(sequence));
	}
	for (auto const& [compartment_connection_on_neuron, compartment_connection] :
	     compartment_connections) {
		ret.compartment_connections.set(
		    compartment_connection_on_neuron, *compartment_connection.get_section(sequence));
	}
	return std::make_unique<Parameterization>(std::move(ret));
}

size_t Neuron::ParameterSpace::Parameterization::size() const
{
	if (compartments.empty()) {
		return 0;
	}
	std::set<size_t> ret;
	for (auto const& [_, compartment] : compartments) {
		ret.insert(compartment.size());
	}
	for (auto const& [_, compartment_connection] : compartment_connections) {
		ret.insert(compartment_connection.size());
	}
	if (ret.size() > 1) {
		throw std::runtime_error("Neuron parameterization features heterogeneous size.");
	}
	return *ret.begin();
}

std::unique_ptr<grenade::common::PortData> Neuron::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<Neuron::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> Neuron::ParameterSpace::Parameterization::move()
{
	return std::make_unique<Neuron::ParameterSpace::Parameterization>(std::move(*this));
}

bool Neuron::ParameterSpace::Parameterization::is_equal_to(
    grenade::common::PortData const& other) const
{
	const auto* other_cast = dynamic_cast<const Neuron::ParameterSpace::Parameterization*>(&other);

	if (!other_cast) {
		return false;
	}
	return (compartments == other_cast->compartments) &&
	       (compartment_connections == other_cast->compartment_connections);
}

std::ostream& Neuron::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	ios << hate::Indentation("\t");
	for (auto const& [compartment_on_neuron, compartment] : compartments) {
		ios << compartment_on_neuron << ": " << compartment << "\n";
	}
	for (auto const& [compartment_connection_on_neuron, compartment_connection] :
	     compartment_connections) {
		ios << compartment_connection_on_neuron << ": " << compartment_connection << "\n";
	}
	ios << hate::Indentation("\t");
	ios << ")";
	return os;
}


size_t Neuron::ParameterSpace::size() const
{
	if (compartments.empty()) {
		return 0;
	}
	std::set<size_t> ret;
	for (auto const& [_, compartment] : compartments) {
		ret.insert(compartment.size());
	}
	for (auto const& [_, compartment_connection] : compartment_connections) {
		ret.insert(compartment_connection.size());
	}
	if (ret.size() > 1) {
		throw std::runtime_error("Neuron parameter space features heterogeneous size.");
	}
	return *ret.begin();
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
Neuron::ParameterSpace::get_section(grenade::common::MultiIndexSequence const& sequence) const
{
	ParameterSpace ret;
	for (auto const& [compartment_on_neuron, compartment] : compartments) {
		ret.compartments.emplace(compartment_on_neuron, compartment.get_section(sequence));
	}
	for (auto const& [compartment_connection_on_neuron, compartment_connection] :
	     compartment_connections) {
		ret.compartment_connections.set(
		    compartment_connection_on_neuron, *compartment_connection.get_section(sequence));
	}
	return std::make_unique<ParameterSpace>(std::move(ret));
}

bool Neuron::ParameterSpace::valid(
    size_t /* input_port_on_cell */,
    grenade::common::Population::Cell::ParameterSpace::Parameterization const& parameterization)
    const
{
	if (auto const parameterization_ptr = dynamic_cast<Parameterization const*>(&parameterization);
	    parameterization_ptr) {
		for (auto [compartment, compartment_parameterization] :
		     parameterization_ptr->compartments) {
			if (!compartments.at(compartment).valid(compartment_parameterization)) {
				return false;
			}
		}
		for (auto [compartment_connection, compartment_connection_parameterization] :
		     parameterization_ptr->compartment_connections) {
			if (!compartment_connections.get(compartment_connection)
			         .valid(compartment_connection_parameterization)) {
				return false;
			}
		}
		return true;
	}
	LOG4CXX_DEBUG(
	    log4cxx::Logger::getLogger("grenade.vx.network.abstract.Neuron"),
	    "Wrong parameterization type supplied.");
	return false;
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> Neuron::ParameterSpace::copy()
    const
{
	return std::make_unique<Neuron::ParameterSpace>(*this);
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> Neuron::ParameterSpace::move()
{
	return std::make_unique<Neuron::ParameterSpace>(std::move(*this));
}

bool Neuron::ParameterSpace::is_equal_to(
    grenade::common::Population::Cell::ParameterSpace const& other) const
{
	const auto* other_cast = dynamic_cast<const Neuron::ParameterSpace*>(&other);

	if (!other_cast) {
		return false;
	}
	return (compartments == other_cast->compartments) &&
	       (compartment_connections == other_cast->compartment_connections);
}

std::ostream& Neuron::ParameterSpace::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "ParameterSpace(\n";
	ios << hate::Indentation("\t");
	for (auto const& [compartment_on_neuron, compartment] : compartments) {
		ios << compartment_on_neuron << ": " << compartment << "\n";
	}
	for (auto const& [compartment_connection_on_neuron, compartment_connection] :
	     compartment_connections) {
		ios << compartment_connection_on_neuron << ": " << compartment_connection << "\n";
	}
	ios << hate::Indentation("\t");
	ios << ")";
	return os;
}


grenade::common::CompartmentOnNeuron Neuron::add_compartment(Compartment const& compartment)
{
	return this->add_vertex(compartment);
}

void Neuron::remove_compartment(grenade::common::CompartmentOnNeuron descriptor)
{
	this->remove_vertex(descriptor);
}

CompartmentConnectionOnNeuron Neuron::add_compartment_connection(
    grenade::common::CompartmentOnNeuron source,
    grenade::common::CompartmentOnNeuron target,
    CompartmentConnection const& edge)
{
	return this->add_edge(source, target, edge);
}

void Neuron::remove_compartment_connection(CompartmentConnectionOnNeuron descriptor)
{
	this->remove_edge(descriptor);
}

Compartment const& Neuron::get(grenade::common::CompartmentOnNeuron const& descriptor) const
{
	return Graph::get(descriptor);
}

void Neuron::set(
    grenade::common::CompartmentOnNeuron const& descriptor, Compartment const& compartment)
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

size_t Neuron::get_compartment_degree(grenade::common::CompartmentOnNeuron const& descriptor) const
{
	assert(Graph::out_degree(descriptor) == Graph::in_degree(descriptor));
	return Graph::out_degree(descriptor);
}

grenade::common::CompartmentOnNeuron Neuron::get_max_degree_compartment() const
{
	grenade::common::CompartmentOnNeuron compartment_max_degree = *(compartments().begin());
	for (auto compartment : compartments()) {
		if (get_compartment_degree(compartment) > get_compartment_degree(compartment_max_degree)) {
			compartment_max_degree = compartment;
		}
	}
	return compartment_max_degree;
}

grenade::common::CompartmentOnNeuron Neuron::source(
    CompartmentConnectionOnNeuron const& descriptor) const
{
	return Graph::source(descriptor);
}

grenade::common::CompartmentOnNeuron Neuron::target(
    CompartmentConnectionOnNeuron const& descriptor) const
{
	return Graph::target(descriptor);
}

// Iterators over Compartments
boost::iterator_range<Neuron::CompartmentIterator> Neuron::compartments() const
{
	return this->vertices();
}

boost::iterator_range<Neuron::CompartmentConnectionIterator> Neuron::compartment_connections() const
{
	return this->edges();
}

boost::iterator_range<Neuron::AdjacencyIterator> Neuron::adjacent_compartments(
    grenade::common::CompartmentOnNeuron const& descriptor) const
{
	return this->adjacent_vertices(descriptor);
}

std::map<grenade::common::CompartmentOnNeuron, grenade::common::CompartmentOnNeuron>
Neuron::isomorphism(Neuron const& other) const
{
	return Graph::isomorphism(other);
}

bool Neuron::has_equal_morphology(Neuron const& other) const
{
	if (num_compartments() != other.num_compartments()) {
		return false;
	}

	for (auto compartment : compartments()) {
		auto neighbours = adjacent_compartments(compartment);
		auto neighbours_other = other.adjacent_compartments(compartment);

		std::set<grenade::common::CompartmentOnNeuron> neighbours_set(
		    neighbours.begin(), neighbours.end());
		std::set<grenade::common::CompartmentOnNeuron> neighbours_other_set(
		    neighbours_other.begin(), neighbours_other.end());

		if (neighbours_set != neighbours_other_set) {
			return false;
		}
	}
	return true;
}

std::map<grenade::common::CompartmentOnNeuron::value_type, size_t>
Neuron::get_compartment_index_map() const
{
	std::map<grenade::common::CompartmentOnNeuron::value_type, size_t> mapping;
	size_t index = 0;
	for (auto compartment : compartments()) {
		mapping.emplace(compartment.value(), index);
		index++;
	}
	return mapping;
}

bool Neuron::neighbour(
    grenade::common::CompartmentOnNeuron const& source,
    grenade::common::CompartmentOnNeuron const& target) const
{
	for (auto compartment : adjacent_compartments(source)) {
		if (compartment == target) {
			return true;
		}
	}
	return false;
}


size_t Neuron::branch_size(
    grenade::common::CompartmentOnNeuron const& compartment,
    std::set<grenade::common::CompartmentOnNeuron>& marked_compartments) const
{
	size_t size = 1;
	marked_compartments.emplace(compartment);

	for (auto adjacent_compartment : adjacent_compartments(compartment)) {
		if (marked_compartments.contains(adjacent_compartment)) {
			continue;
		}
		size += branch_size(adjacent_compartment, marked_compartments);
	}
	return size;
}

bool Neuron::is_chain(
    grenade::common::CompartmentOnNeuron const& compartment,
    std::set<grenade::common::CompartmentOnNeuron>& marked_compartments) const
{
	marked_compartments.emplace(compartment);

	size_t non_leaf_neighbours = 0;
	for (auto adjacent_compartment : adjacent_compartments(compartment)) {
		if (marked_compartments.contains(adjacent_compartment)) {
			continue;
		}
		if (get_compartment_degree(adjacent_compartment) > 1) {
			non_leaf_neighbours++;
		}
		if ((non_leaf_neighbours > 1) || !is_chain(adjacent_compartment, marked_compartments)) {
			return false;
		}
	}
	return true;
}

std::vector<grenade::common::CompartmentOnNeuron> Neuron::branch_compartments(
    grenade::common::CompartmentOnNeuron const& compartment,
    grenade::common::CompartmentOnNeuron const& blacklist_compartment) const
{
	std::vector<grenade::common::CompartmentOnNeuron> branch;
	branch.push_back(compartment);

	std::set<grenade::common::CompartmentOnNeuron> marked_compartments;
	marked_compartments.emplace(compartment);
	marked_compartments.emplace(blacklist_compartment);

	std::stack<grenade::common::CompartmentOnNeuron> compartment_queue;
	compartment_queue.push(compartment);

	while (!compartment_queue.empty()) {
		auto current_compartment = compartment_queue.top();
		compartment_queue.pop();

		for (auto adjacent_compartment : adjacent_compartments(current_compartment)) {
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
    grenade::common::CompartmentOnNeuron const& compartment,
    std::set<grenade::common::CompartmentOnNeuron> neighbours_whitelist) const
{
	CompartmentNeighbours neighbours;

	std::set<grenade::common::CompartmentOnNeuron> marked_compartments;
	marked_compartments.emplace(compartment);

	for (auto adjacent_compartment : adjacent_compartments(compartment)) {
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

bool Neuron::contains(grenade::common::CompartmentOnNeuron const& descriptor) const
{
	return Graph::contains(descriptor);
}

bool Neuron::valid(grenade::common::Population::Cell::ParameterSpace const& parameter_space) const
{
	if (auto const neuron_parameter_space = dynamic_cast<ParameterSpace const*>(&parameter_space);
	    neuron_parameter_space) {
		for (auto& [compartment, compartment_parameter_space] :
		     neuron_parameter_space->compartments) {
			if (!get(compartment).valid(compartment_parameter_space)) {
				return false;
			}
		}
	} else {
		return false;
	}
	return true;
}

bool Neuron::is_connected() const
{
	return Graph::is_connected();
}

bool Neuron::valid(
    size_t /* input_port_on_cell */,
    grenade::common::Population::Cell::Dynamics const& /* dynamics */) const
{
	return false;
}


// Writes neuron topology in graphviz format to be plotted later
void Neuron::write_graphviz(std::string filename, std::string name)
{
	std::ofstream file;
	file.open(filename);
	file << "graph " << name << " {\n";
	for (auto connection : compartment_connections()) {
		auto compartment_a = source(connection);
		auto compartment_b = target(connection);
		file << compartment_a << "--" << compartment_b << "\n";
	}
	file << "}\n";
	file.close();
}

bool Neuron::requires_time_domain() const
{
	return true;
}

bool Neuron::is_partitionable() const
{
	return false;
}

bool Neuron::valid(grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const
{
	return dynamic_cast<ClockCycleTimeDomainRuntimes const*>(&time_domain_runtimes) != nullptr;
}

std::vector<grenade::common::Vertex::Port> Neuron::get_input_ports() const
{
	std::vector<grenade::common::Vertex::Port> ret;

	for (auto const& compartment_on_neuron : compartments()) {
		auto const& compartment = get(compartment_on_neuron);
		for (auto const& [mechanism_on_compartment, mechanism] : compartment.mechanisms) {
			auto mechanism_ports = mechanism.get_input_ports();
			for (auto&& mechanism_port : mechanism_ports) {
				auto channels =
				    grenade::common::CuboidMultiIndexSequence(
				        {1, 1},
				        grenade::common::MultiIndex(
				            {compartment_on_neuron.value(), mechanism_on_compartment.value()}),
				        {grenade::common::CompartmentOnNeuronDimensionUnit(),
				         MechanismOnCompartmentDimensionUnit()})
				        .cartesian_product(mechanism_port.get_channels());
				auto const port_it =
				    std::find_if(ret.begin(), ret.end(), [&mechanism_port](auto const& port) {
					    // TODO: costly
					    auto port_copy = port;
					    port_copy.set_channels(grenade::common::ListMultiIndexSequence());
					    auto mechanism_port_copy = mechanism_port;
					    mechanism_port_copy.set_channels(grenade::common::ListMultiIndexSequence());
					    return port_copy == mechanism_port_copy;
				    });
				if (port_it == ret.end()) {
					mechanism_port.set_channels(*channels);
					ret.emplace_back(std::move(mechanism_port));
				} else {
					auto& port = *port_it;
					auto port_channel_elements = port.get_channels().get_elements();
					auto channel_elements = channels->get_elements();
					if (port.get_channels().get_dimension_units() !=
					    channels->get_dimension_units()) {
						throw std::runtime_error("Mechanism port of equal type has different "
						                         "dimension units than already existing port.");
					}
					if (!port.get_channels().is_disjunct(*channels)) {
						throw std::runtime_error(
						    "Mechanism port has channel overlap with already existing port.");
					}
					port_channel_elements.insert(
					    port_channel_elements.end(), channel_elements.begin(),
					    channel_elements.end());
					port.set_channels(grenade::common::ListMultiIndexSequence(
					    std::move(port_channel_elements),
					    port.get_channels().get_dimension_units()));
				}
			}
		}
	}

	ret.push_back(grenade::common::Vertex::Port(
	    ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})})));

	return ret;
}

std::vector<grenade::common::Vertex::Port> Neuron::get_output_ports() const
{
	std::vector<grenade::common::Vertex::Port> ret;

	for (auto const& compartment_on_neuron : compartments()) {
		auto const& compartment = get(compartment_on_neuron);
		for (auto const& [mechanism_on_compartment, mechanism] : compartment.mechanisms) {
			auto mechanism_ports = mechanism.get_output_ports();
			for (auto&& mechanism_port : mechanism_ports) {
				auto channels =
				    grenade::common::CuboidMultiIndexSequence(
				        {1, 1},
				        grenade::common::MultiIndex(
				            {compartment_on_neuron.value(), mechanism_on_compartment.value()}),
				        {grenade::common::CompartmentOnNeuronDimensionUnit(),
				         MechanismOnCompartmentDimensionUnit()})
				        .cartesian_product(mechanism_port.get_channels());
				auto const port_it =
				    std::find_if(ret.begin(), ret.end(), [&mechanism_port](auto const& port) {
					    // TODO: costly
					    auto port_copy = port;
					    port_copy.set_channels(grenade::common::ListMultiIndexSequence());
					    auto mechanism_port_copy = mechanism_port;
					    mechanism_port_copy.set_channels(grenade::common::ListMultiIndexSequence());
					    return port_copy == mechanism_port_copy;
				    });
				if (port_it == ret.end()) {
					mechanism_port.set_channels(*channels);
					ret.emplace_back(std::move(mechanism_port));
				} else {
					auto& port = *port_it;
					auto port_channel_elements = port.get_channels().get_elements();
					auto channel_elements = channels->get_elements();
					if (port.get_channels().get_dimension_units() !=
					    channels->get_dimension_units()) {
						throw std::runtime_error("Mechanism port of equal type has different "
						                         "dimension units than already existing port.");
					}
					if (!port.get_channels().is_disjunct(*channels)) {
						throw std::runtime_error(
						    "Mechanism port has channel overlap with already existing port.");
					}
					port_channel_elements.insert(
					    port_channel_elements.end(), channel_elements.begin(),
					    channel_elements.end());
					port.set_channels(grenade::common::ListMultiIndexSequence(
					    std::move(port_channel_elements),
					    port.get_channels().get_dimension_units()));
				}
			}
		}
	}
	return ret;
}

std::unique_ptr<grenade::common::Population::Cell> Neuron::copy() const
{
	return std::make_unique<Neuron>(*this);
}

std::unique_ptr<grenade::common::Population::Cell> Neuron::move()
{
	return std::make_unique<Neuron>(std::move(*this));
}

bool Neuron::is_equal_to(grenade::common::Population::Cell const& other) const
{
	return Graph::operator==(static_cast<Neuron const&>(other));
}

std::ostream& Neuron::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Neuron(\n";
	ios << hate::Indentation("\t");
	ios << "Number of compartments: " << num_compartments() << "\n";

	ios << "Connections: " << num_compartment_connections() << " ["
	    << hate::join(
	           compartment_connections(), ", ",
	           [this](auto const& v) {
		           std::stringstream ss;
		           ss << "(" << source(v).value() << ", " << target(v).value() << ")";
		           return ss.str();
	           })
	    << "]\n";

	// print compartments without connections
	std::vector<int> isolated;
	for (auto const& compartment : compartments()) {
		if (get_compartment_degree(compartment) == 0) {
			isolated.push_back(compartment.value());
		}
	}
	if (!isolated.empty()) {
		ios << "Without connection: " << isolated.size() << " [" << hate::join(isolated, ", ")
		    << "]\n";
	}

	ios << hate::Indentation();
	ios << ")";

	return os;
}

} // namespace grenade::vx::network::abstract
