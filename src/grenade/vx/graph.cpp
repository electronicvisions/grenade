#include "grenade/vx/graph.h"

#include <iterator>
#include <sstream>

#include <boost/graph/exception.hpp>
#include <boost/graph/topological_sort.hpp>

#include <log4cxx/logger.h>

#include "grenade/vx/connection_restriction.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/input.h"
#include "hate/timer.h"

namespace grenade::vx {

Graph::Graph(bool enable_acyclicity_check) :
    m_enable_acyclicity_check(enable_acyclicity_check),
    m_graph(),
    m_execution_instance_graph(),
    m_edge_property_map(),
    m_vertex_property_map(),
    m_vertex_descriptor_map(),
    m_execution_instance_map(),
    m_logger(log4cxx::Logger::getLogger("grenade.Graph"))
{}

Graph::vertex_descriptor Graph::add(
    vertex_descriptor const vertex_reference,
    coordinate::ExecutionInstance const execution_instance,
    std::vector<Input> const inputs)
{
	return add<>(vertex_reference, execution_instance, inputs);
}

void Graph::add_edges(
    vertex_descriptor descriptor,
    coordinate::ExecutionInstance const& execution_instance,
    std::vector<Input> const& inputs)
{
	// add execution instance (if not already present)
	vertex_descriptor execution_instance_descriptor;
	auto const execution_instance_map_it = m_execution_instance_map.right.find(execution_instance);
	if (execution_instance_map_it == m_execution_instance_map.right.end()) {
		execution_instance_descriptor = boost::add_vertex(m_execution_instance_graph);
		m_execution_instance_map.insert({execution_instance_descriptor, execution_instance});
	} else {
		execution_instance_descriptor = execution_instance_map_it->second;
	}

	// add mapping between execution instance descriptor and vertex descriptor
	m_vertex_descriptor_map.insert({descriptor, execution_instance_descriptor});

	bool new_edges_across_execution_instances = false;
	for (auto const& input : inputs) {
		// add edge between vertex descriptors
		{
			auto const [edge, success] = boost::add_edge(input.descriptor, descriptor, m_graph);
			if (!success) {
				throw std::logic_error("Adding edge to graph unsuccessful.");
			}
			auto [_, property_success] = m_edge_property_map.insert({edge, input.port_restriction});
			if (!property_success) {
				throw std::logic_error("Adding edge property unsuccessful.");
			}
		}
		// maybe add edge to execution instance graph
		auto const input_execution_instance_descriptor =
		    m_vertex_descriptor_map.left.at(input.descriptor);
		if (input_execution_instance_descriptor != execution_instance_descriptor) {
			// only add new edge if not already present
			auto const in_edges =
			    boost::in_edges(execution_instance_descriptor, m_execution_instance_graph);
			if (std::none_of(in_edges.first, in_edges.second, [&](auto const& i) {
				    return boost::source(i, m_execution_instance_graph) ==
				           input_execution_instance_descriptor;
			    })) {
				new_edges_across_execution_instances = true;
				auto const [_, success] = boost::add_edge(
				    input_execution_instance_descriptor, execution_instance_descriptor,
				    m_execution_instance_graph);
				if (!success) {
					throw std::logic_error("Adding edge to execution instance graph unsuccessful.");
				}
			}
		}
	}

	// maybe check for acyclicity in execution instance graph
	if (m_enable_acyclicity_check && new_edges_across_execution_instances) {
		if (!is_acyclic_execution_instance_graph()) {
			throw std::runtime_error("Execution instance graph cyclic.");
		}
	}
}

void Graph::add_log(
    vertex_descriptor descriptor,
    coordinate::ExecutionInstance const& execution_instance,
    hate::Timer const& timer)
{
	auto const log = [&](auto const& v) {
		LOG4CXX_TRACE(
		    m_logger, "add(): Added vertex " << descriptor << " at " << execution_instance << " in "
		                                     << timer.print()
		                                     << " with configuration: " << std::endl
		                                     << v << ".");
	};
	std::visit(log, get_vertex_property(descriptor));
}

Graph::graph_type const& Graph::get_graph() const
{
	return m_graph;
}

Graph::graph_type const& Graph::get_execution_instance_graph() const
{
	return m_execution_instance_graph;
}

Vertex const& Graph::get_vertex_property(vertex_descriptor const descriptor) const
{
	return *(m_vertex_property_map.at(descriptor));
}

Graph::edge_property_map_type const& Graph::get_edge_property_map() const
{
	return m_edge_property_map;
}

Graph::execution_instance_map_type const& Graph::get_execution_instance_map() const
{
	return m_execution_instance_map;
}

Graph::vertex_descriptor_map_type const& Graph::get_vertex_descriptor_map() const
{
	return m_vertex_descriptor_map;
}

namespace detail {

/**
 * Output iterator dropping all mutable operations.
 */
template <typename T>
class NullOutputIterator : public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
	NullOutputIterator& operator=(T const&)
	{
		return *this;
	}
	NullOutputIterator& operator*()
	{
		return *this;
	}
	NullOutputIterator& operator++()
	{
		return *this;
	}
	NullOutputIterator operator++(int)
	{
		return *this;
	}
};

} // namespace detail

bool Graph::is_acyclic_execution_instance_graph() const
{
	try {
		boost::topological_sort(
		    m_execution_instance_graph, detail::NullOutputIterator<vertex_descriptor>{});
	} catch (boost::not_a_dag const&) {
		return false;
	}
	return true;
}

namespace {

bool value_equal(Graph::graph_type const& a, Graph::graph_type const& b)
{
	return std::equal(
	           boost::vertices(a).first, boost::vertices(a).second, boost::vertices(b).first,
	           boost::vertices(b).second) &&
	       std::equal(
	           boost::edges(a).first, boost::edges(a).second, boost::edges(b).first,
	           boost::edges(b).second, [&](auto const& aa, auto const& bb) {
		           return (boost::source(aa, a) == boost::source(bb, b)) &&
		                  (boost::target(aa, a) == boost::target(aa, b));
	           });
}

bool value_equal(Graph::vertex_property_map_type const& a, Graph::vertex_property_map_type const& b)
{
	return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](auto const& aa, auto const& bb) {
		assert(aa && bb);
		return *aa == *bb;
	});
}

bool value_equal(
    Graph::edge_property_map_type const& a,
    Graph::edge_property_map_type const& b,
    Graph::graph_type const& ga,
    Graph::graph_type const& gb)
{
	// expect equal source, target, position in boost::edges(graph) and value
	auto const get_position_map = [](Graph::edge_property_map_type const& m,
	                                 Graph::graph_type const& g) {
		std::map<size_t, std::optional<PortRestriction>> position_map;
		for (auto const& p : m) {
			auto const epos = std::find(boost::edges(g).first, boost::edges(g).second, p.first);
			assert(epos != boost::edges(g).second);
			position_map.insert({std::distance(boost::edges(g).first, epos), p.second});
		}
		return position_map;
	};

	std::vector<Graph::edge_descriptor> aedges(boost::edges(ga).first, boost::edges(ga).second);
	std::vector<Graph::edge_descriptor> bedges(boost::edges(gb).first, boost::edges(gb).second);
	auto const compare = [&](auto const& aa, auto const& bb) {
		return (boost::source(aedges.at(aa.first), ga) == boost::source(bedges.at(bb.first), gb)) &&
		       (boost::target(aedges.at(aa.first), ga) == boost::target(bedges.at(bb.first), gb)) &&
		       (aa.first == bb.first) && (aa.second == bb.second);
	};

	auto const apos = get_position_map(a, ga);
	auto const bpos = get_position_map(b, gb);
	return std::equal(apos.begin(), apos.end(), bpos.begin(), bpos.end(), compare);
}

} // namespace

bool Graph::operator==(Graph const& other) const
{
	return (m_enable_acyclicity_check == other.m_enable_acyclicity_check) &&
	       value_equal(m_graph, other.m_graph) &&
	       value_equal(m_execution_instance_graph, other.m_execution_instance_graph) &&
	       value_equal(m_edge_property_map, other.m_edge_property_map, m_graph, other.m_graph) &&
	       value_equal(m_vertex_property_map, other.m_vertex_property_map) &&
	       (m_vertex_descriptor_map == other.m_vertex_descriptor_map) &&
	       (m_execution_instance_map == other.m_execution_instance_map);
}

bool Graph::operator!=(Graph const& other) const
{
	return !(*this == other);
}

template <bool VariadicInput>
void Graph::check_inputs_size(size_t const vertex_inputs_size, size_t const inputs_size)
{
	if (inputs_size < vertex_inputs_size) {
		std::stringstream ss;
		ss << "Number of supplied inputs (" << inputs_size
		   << ") is smaller than the expected number of "
		   << "(" << vertex_inputs_size << ") from vertex.";
		throw std::runtime_error(ss.str());
	}

	if constexpr (!VariadicInput) {
		if (inputs_size > vertex_inputs_size) {
			std::stringstream ss;
			ss << "Number of supplied inputs (" << inputs_size
			   << ") is larger than the expected number of "
			   << "(" << vertex_inputs_size << ") from vertex.";
			throw std::runtime_error(ss.str());
		}
	}
}

template <typename VertexPort, typename InputVertexPort>
void Graph::check_input_port(
    VertexPort const& vertex_port,
    InputVertexPort const& input_vertex_port,
    std::optional<PortRestriction> const& input_vertex_port_restriction)
{
	if (input_vertex_port_restriction) {
		// check incoming port restriction is valid
		if (!input_vertex_port_restriction->is_restriction_of(input_vertex_port)) {
			std::stringstream ss;
			ss << *input_vertex_port_restriction << " is not a restriction of provided port "
			   << input_vertex_port << " of input vertex.";
			throw std::runtime_error(ss.str());
		}

		// check incoming restricted port matches positional input port
		if ((vertex_port.type != input_vertex_port.type) ||
		    (vertex_port.size != input_vertex_port_restriction->size())) {
			std::stringstream ss;
			ss << "Vertex port " << vertex_port << " does not match provided port "
			   << input_vertex_port << " with " << *input_vertex_port_restriction
			   << " of input vertex.";
			throw std::runtime_error(ss.str());
		}
	} else {
		// check incoming port matches positional input port
		if (vertex_port != input_vertex_port) {
			std::stringstream ss;
			ss << "Vertex port " << vertex_port << " does not match provided port "
			   << input_vertex_port << " of input vertex.";
			throw std::runtime_error(ss.str());
		}
	}
}

template <typename Vertex, typename InputVertex>
void Graph::check_supports_input_from(
    Vertex const& vertex,
    InputVertex const& input_vertex,
    std::optional<PortRestriction> const& input_port_restriction)
{
	if (!supports_input_from(vertex, input_vertex, input_port_restriction)) {
		throw std::runtime_error("Vertex does not support input from incoming vertex.");
	}
}

template <typename Vertex, typename InputVertex>
void Graph::check_execution_instances(
    Vertex const&,
    InputVertex const& input_vertex,
    coordinate::ExecutionInstance const& vertex_execution_instance,
    coordinate::ExecutionInstance const& input_vertex_execution_instance)
{
	auto const connection_type = input_vertex.output().type;
	auto const connection_type_can_connect =
	    std::find(
	        can_connect_different_execution_instances.begin(),
	        can_connect_different_execution_instances.end(),
	        connection_type) != std::cend(can_connect_different_execution_instances);
	if (connection_type_can_connect && Vertex::can_connect_different_execution_instances &&
	    InputVertex::can_connect_different_execution_instances) {
		if (vertex_execution_instance == input_vertex_execution_instance) {
			throw std::runtime_error("Connection does not go forward in time dimension.");
		}
	} else {
		if (vertex_execution_instance != input_vertex_execution_instance) {
			throw std::runtime_error("Connection between specified vertices not allowed "
			                         "over different times.");
		}
	}
}

void Graph::check_inputs(
    Vertex const& vertex,
    coordinate::ExecutionInstance const& execution_instance,
    std::vector<Input> const& inputs)
{
	auto const checker = [&](auto const& v) {
		auto const vertex_inputs = v.inputs();

		check_inputs_size<v.variadic_input>(vertex_inputs.size(), inputs.size());

		for (size_t i = 0; i < inputs.size(); ++i) {
			auto const& input = inputs.at(i);
			size_t const input_index = std::min(i, vertex_inputs.size() - 1);

			auto const check_single_input = [&](auto const& w) {
				check_input_port(vertex_inputs.at(input_index), w.output(), input.port_restriction);

				check_supports_input_from(v, w, input.port_restriction);

				auto const& input_execution_instance = m_execution_instance_map.left.at(
				    m_vertex_descriptor_map.left.at(input.descriptor));
				check_execution_instances(v, w, execution_instance, input_execution_instance);
			};

			std::visit(check_single_input, get_vertex_property(input.descriptor));
		}
	};

	std::visit(checker, vertex);
}

} // namespace grenade::vx
