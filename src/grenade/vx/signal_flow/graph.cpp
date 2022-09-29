#include "grenade/vx/signal_flow/graph.h"

#include <iterator>
#include <ostream>
#include <sstream>

#include <boost/graph/exception.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/topological_sort.hpp>

#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>
#include <log4cxx/logger.h>

#include "grenade/cerealization.h"
#include "grenade/vx/signal_flow/execution_instance.h"
#include "grenade/vx/signal_flow/input.h"
#include "grenade/vx/signal_flow/supports_input_from.h"
#include "hate/timer.h"
#include "hate/type_index.h"
#include "hate/type_traits.h"

namespace cereal {

template <class Archive>
void CEREAL_SAVE_FUNCTION_NAME(
    Archive& ar, grenade::vx::signal_flow::Graph::execution_instance_map_type const& data)
{
	std::vector<std::pair<
	    grenade::vx::signal_flow::Graph::vertex_descriptor,
	    grenade::vx::signal_flow::ExecutionInstance>>
	    values;
	for (auto const& p : data.left) {
		values.push_back(std::pair{p.first, p.second});
	}
	ar(CEREAL_NVP(values));
}

template <class Archive>
void CEREAL_LOAD_FUNCTION_NAME(
    Archive& ar, grenade::vx::signal_flow::Graph::execution_instance_map_type& data)
{
	std::vector<std::pair<
	    grenade::vx::signal_flow::Graph::vertex_descriptor,
	    grenade::vx::signal_flow::ExecutionInstance>>
	    values;
	ar(CEREAL_NVP(values));
	for (auto const& p : values) {
		data.left.insert({p.first, p.second});
	}
}

template <class Archive>
void CEREAL_SAVE_FUNCTION_NAME(
    Archive& ar, grenade::vx::signal_flow::Graph::vertex_descriptor_map_type const& data)
{
	std::vector<std::pair<
	    grenade::vx::signal_flow::Graph::vertex_descriptor,
	    grenade::vx::signal_flow::Graph::vertex_descriptor>>
	    values;
	for (auto const& p : data) {
		values.push_back(std::pair{p.left, p.right});
	}
	ar(CEREAL_NVP(values));
}

template <class Archive>
void CEREAL_LOAD_FUNCTION_NAME(
    Archive& ar, grenade::vx::signal_flow::Graph::vertex_descriptor_map_type& data)
{
	std::vector<std::pair<
	    grenade::vx::signal_flow::Graph::vertex_descriptor,
	    grenade::vx::signal_flow::Graph::vertex_descriptor>>
	    values;
	ar(CEREAL_NVP(values));
	for (auto const& p : values) {
		data.left.insert({p.first, p.second});
	}
}

template <class Archive>
void CEREAL_SAVE_FUNCTION_NAME(Archive& ar, grenade::vx::signal_flow::Graph::graph_type const& data)
{
	int const num_vertices = boost::num_vertices(data);
	int const num_edges = boost::num_edges(data);
	ar(CEREAL_NVP(num_vertices));
	ar(CEREAL_NVP(num_edges));

	// assign indices to vertices
	std::map<grenade::vx::signal_flow::Graph::vertex_descriptor, int> indices;
	int num = 0;
	for (auto const vi : boost::make_iterator_range(boost::vertices(data))) {
		indices[vi] = num++;
	}

	// write edges
	for (auto const ei : boost::make_iterator_range(boost::edges(data))) {
		ar(CEREAL_NVP_("u", indices.at(boost::source(ei, data))));
		ar(CEREAL_NVP_("v", indices.at(boost::target(ei, data))));
	}
}

template <class Archive>
void CEREAL_LOAD_FUNCTION_NAME(Archive& ar, grenade::vx::signal_flow::Graph::graph_type& data)
{
	int num_vertices;
	int num_edges;
	ar(CEREAL_NVP(num_vertices));
	ar(CEREAL_NVP(num_edges));

	std::vector<grenade::vx::signal_flow::Graph::vertex_descriptor> vertices(num_vertices);
	int i = 0;
	while (num_vertices-- > 0) {
		auto const v = boost::add_vertex(data);
		vertices[i++] = v;
	}
	while (num_edges-- > 0) {
		int u;
		int v;
		ar(CEREAL_NVP(u));
		ar(CEREAL_NVP(v));
		auto const [_, inserted] = boost::add_edge(vertices.at(u), vertices.at(v), data);
		assert(inserted);
	}
}

} // namespace cereal

namespace grenade::vx::signal_flow {

Graph::Graph(bool enable_acyclicity_check) :
    m_enable_acyclicity_check(enable_acyclicity_check),
    m_graph(std::make_unique<graph_type>()),
    m_execution_instance_graph(std::make_unique<graph_type>()),
    m_edge_property_map(),
    m_vertex_property_map(),
    m_vertex_descriptor_map(),
    m_execution_instance_map(),
    m_logger(log4cxx::Logger::getLogger("grenade.Graph"))
{}

namespace {

/**
 * Copy edge property map with given reference and graphs.
 * @param other Other property map to copy
 * @param tg Graph for which to produce copy of property map
 * @param og Other graph related to property map to copy
 */
Graph::edge_property_map_type copy_edge_property_map(
    Graph::edge_property_map_type const& other,
    Graph::graph_type const& tg,
    Graph::graph_type const& og)
{
	Graph::edge_property_map_type ret;
	// edge_descriptor is not invariant under copy, but its position in boost::edges(graph) is.
	std::vector<std::pair<size_t, std::optional<PortRestriction>>> edge_property_list;
	std::vector<Graph::edge_descriptor> other_edges(
	    boost::edges(og).first, boost::edges(og).second);
	std::vector<Graph::edge_descriptor> edges(boost::edges(tg).first, boost::edges(tg).second);
	for (auto const& p : other) {
		auto const other_epos = std::find(other_edges.begin(), other_edges.end(), p.first);
		assert(other_epos != other_edges.end());
		ret.insert({edges.at(std::distance(other_edges.begin(), other_epos)), p.second});
	}
	return ret;
}

} // namespace

Graph::Graph(Graph const& other) :
    m_enable_acyclicity_check(other.m_enable_acyclicity_check),
    m_graph(other.m_graph ? std::make_unique<graph_type>(*other.m_graph) : nullptr),
    m_execution_instance_graph(
        other.m_execution_instance_graph
            ? std::make_unique<graph_type>(*other.m_execution_instance_graph)
            : nullptr),
    m_edge_property_map(
        (m_graph && other.m_graph)
            ? copy_edge_property_map(other.m_edge_property_map, *m_graph, *other.m_graph)
            : other.m_edge_property_map),
    m_vertex_property_map(other.m_vertex_property_map),
    m_vertex_descriptor_map(other.m_vertex_descriptor_map),
    m_execution_instance_map(other.m_execution_instance_map),
    m_logger(log4cxx::Logger::getLogger("grenade.Graph"))
{}

Graph::Graph(Graph&& other) :
    m_enable_acyclicity_check(other.m_enable_acyclicity_check),
    m_graph(std::move(other.m_graph)),
    m_execution_instance_graph(std::move(other.m_execution_instance_graph)),
    m_edge_property_map(std::move(other.m_edge_property_map)),
    m_vertex_property_map(std::move(other.m_vertex_property_map)),
    m_vertex_descriptor_map(std::move(other.m_vertex_descriptor_map)),
    m_execution_instance_map(std::move(other.m_execution_instance_map)),
    m_logger(log4cxx::Logger::getLogger("grenade.Graph"))
{}

Graph& Graph::operator=(Graph const& other)
{
	if (this == &other) {
		return *this;
	}
	m_enable_acyclicity_check = other.m_enable_acyclicity_check;
	m_graph = other.m_graph ? std::make_unique<graph_type>(*other.m_graph) : nullptr;
	m_execution_instance_graph =
	    other.m_execution_instance_graph
	        ? std::make_unique<graph_type>(*other.m_execution_instance_graph)
	        : nullptr;
	m_edge_property_map =
	    (m_graph && other.m_graph)
	        ? copy_edge_property_map(other.m_edge_property_map, *m_graph, *other.m_graph)
	        : other.m_edge_property_map;
	m_vertex_property_map = other.m_vertex_property_map;
	m_vertex_descriptor_map = other.m_vertex_descriptor_map;
	m_execution_instance_map = other.m_execution_instance_map;
	return *this;
}

Graph& Graph::operator=(Graph&& other)
{
	if (this == &other) {
		return *this;
	}
	m_enable_acyclicity_check = other.m_enable_acyclicity_check;
	m_graph = std::move(other.m_graph);
	m_execution_instance_graph = std::move(other.m_execution_instance_graph);
	m_edge_property_map = std::move(other.m_edge_property_map);
	m_vertex_property_map = std::move(other.m_vertex_property_map);
	m_vertex_descriptor_map = std::move(other.m_vertex_descriptor_map);
	m_execution_instance_map = std::move(other.m_execution_instance_map);
	return *this;
}

Graph::vertex_descriptor Graph::add(
    vertex_descriptor const vertex_reference,
    signal_flow::ExecutionInstance const execution_instance,
    std::vector<Input> const inputs)
{
	return add<>(vertex_reference, execution_instance, inputs);
}

void Graph::add_edges(
    vertex_descriptor descriptor,
    signal_flow::ExecutionInstance const& execution_instance,
    std::vector<Input> const& inputs)
{
	// add execution instance (if not already present)
	vertex_descriptor execution_instance_descriptor;
	auto const execution_instance_map_it = m_execution_instance_map.right.find(execution_instance);
	if (execution_instance_map_it == m_execution_instance_map.right.end()) {
		assert(m_execution_instance_graph);
		execution_instance_descriptor = boost::add_vertex(*m_execution_instance_graph);
		m_execution_instance_map.insert({execution_instance_descriptor, execution_instance});
	} else {
		execution_instance_descriptor = execution_instance_map_it->second;
	}

	// add mapping between execution instance descriptor and vertex descriptor
	m_vertex_descriptor_map.insert({descriptor, execution_instance_descriptor});

	bool new_edges_across_execution_instances = false;
	assert(m_graph);
	assert(m_execution_instance_graph);
	for (auto const& input : inputs) {
		// add edge between vertex descriptors
		{
			auto const [edge, success] = boost::add_edge(input.descriptor, descriptor, *m_graph);
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
			    boost::in_edges(execution_instance_descriptor, *m_execution_instance_graph);
			if (std::none_of(in_edges.first, in_edges.second, [&](auto const& i) {
				    return boost::source(i, *m_execution_instance_graph) ==
				           input_execution_instance_descriptor;
			    })) {
				new_edges_across_execution_instances = true;
				auto const [_, success] = boost::add_edge(
				    input_execution_instance_descriptor, execution_instance_descriptor,
				    *m_execution_instance_graph);
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
    signal_flow::ExecutionInstance const& execution_instance,
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
	assert(m_graph);
	return *m_graph;
}

Graph::graph_type const& Graph::get_execution_instance_graph() const
{
	assert(m_execution_instance_graph);
	return *m_execution_instance_graph;
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
	assert(m_execution_instance_graph);
	try {
		boost::topological_sort(
		    *m_execution_instance_graph, detail::NullOutputIterator<vertex_descriptor>{});
	} catch (boost::not_a_dag const&) {
		return false;
	}
	return true;
}

std::ostream& operator<<(std::ostream& os, Graph const& graph)
{
	// get all different vertex properties by storing their raw pointers
	// this only works because they are stored as shared_ptr
	std::map<size_t, std::map<Vertex const*, Graph::vertex_descriptor>> same_vertex_configs;
	for (auto const descriptor : boost::make_iterator_range(boost::vertices(graph.get_graph()))) {
		auto const& vertex_property = graph.get_vertex_property(descriptor);
		auto& local = same_vertex_configs[vertex_property.index()];
		if (local.contains(&vertex_property)) {
			// update when descriptor is smaller
			if (descriptor < local.at(&vertex_property)) {
				local.at(&vertex_property) = descriptor;
			}
		} else {
			local.insert({&vertex_property, descriptor});
		}
	}
	// get vertex name by type name and index to identify (possibly shared) property
	auto const vertex_name = [graph, same_vertex_configs](
	                             std::ostream& out, Graph::vertex_descriptor const descriptor) {
		auto const& vertex_property = graph.get_vertex_property(descriptor);
		// generate a unique index to the associated vertex property
		// ordering of the index follows the smallest vertex descriptor of same property
		auto const& local = same_vertex_configs.at(vertex_property.index());
		auto const index = local.at(&vertex_property);
		auto const name = std::visit(
		    [index](auto const& v) {
			    return hate::name<hate::remove_all_qualifiers_t<decltype(v)>>() + "(" +
			           std::to_string(index) + ")";
		    },
		    vertex_property);
		out << "[label=\"" << name << "\"]";
	};
	boost::write_graphviz(os, graph.get_graph(), vertex_name);
	return os;
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
	       (m_graph && other.m_graph ? value_equal(*m_graph, *other.m_graph)
	                                 : (m_graph == other.m_graph)) &&
	       (m_execution_instance_graph && other.m_execution_instance_graph
	            ? value_equal(*m_execution_instance_graph, *other.m_execution_instance_graph)
	            : (m_execution_instance_graph == other.m_execution_instance_graph)) &&
	       (m_graph && other.m_graph
	            ? value_equal(
	                  m_edge_property_map, other.m_edge_property_map, *m_graph, *other.m_graph)
	            : m_edge_property_map == other.m_edge_property_map) &&
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
	if constexpr (VariadicInput) {
		if (vertex_inputs_size == 0) {
			throw std::logic_error("Variadic vertex input not supported for input size of zero.");
		}
	}
	size_t const vertex_minimal_inputs_size =
	    vertex_inputs_size - static_cast<size_t>(VariadicInput);
	if (inputs_size < vertex_minimal_inputs_size) {
		std::stringstream ss;
		ss << "Number of supplied inputs (" << inputs_size << ") is smaller than the "
		   << (VariadicInput ? "minimal " : "") << "expected number of "
		   << "(" << vertex_minimal_inputs_size << ") from vertex.";
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
		std::stringstream ss;
		ss << "Vertex(" << vertex << ") does not support input from incoming vertex("
		   << input_vertex << ").";
		throw std::runtime_error(ss.str());
	}
}

template <typename Vertex, typename InputVertex>
void Graph::check_execution_instances(
    Vertex const&,
    InputVertex const& input_vertex,
    signal_flow::ExecutionInstance const& vertex_execution_instance,
    signal_flow::ExecutionInstance const& input_vertex_execution_instance)
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
			throw std::runtime_error(
			    "Connection(" + hate::name<InputVertex>() + " -> " + hate::name<Vertex>() +
			    ") does not go forward in time dimension.");
		}
	} else {
		if (vertex_execution_instance != input_vertex_execution_instance) {
			throw std::runtime_error(
			    "Connection(" + hate::name<InputVertex>() + " -> " + hate::name<Vertex>() +
			    ") not allowed "
			    "over different times.");
		}
	}
}

void Graph::check_inputs(
    Vertex const& vertex,
    signal_flow::ExecutionInstance const& execution_instance,
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

void Graph::update(vertex_descriptor const vertex_reference, Vertex&& vertex)
{
	// check inputs work for updated vertex property
	auto const& execution_instance =
	    m_execution_instance_map.left.at(m_vertex_descriptor_map.left.at(vertex_reference));
	assert(m_graph);
	auto in_edges = boost::in_edges(vertex_reference, *m_graph);
	std::vector<Input> inputs;
	for (auto const& in_edge : boost::make_iterator_range(in_edges)) {
		auto const source = boost::source(in_edge, *m_graph);
		auto const port_restriction = m_edge_property_map.at(in_edge);
		if (port_restriction) {
			inputs.emplace_back(source, *port_restriction);
		} else {
			inputs.emplace_back(source);
		}
	}
	check_inputs(vertex, execution_instance, inputs);
	// update vertex property
	auto old_vertex = std::move(*(m_vertex_property_map.at(vertex_reference)));
	*(m_vertex_property_map.at(vertex_reference)) = std::move(vertex);
	// check inputs for all targets of vertex support updated vertex proprety
	try {
		auto out_edges = boost::out_edges(vertex_reference, *m_graph);
		for (auto const& out_edge : boost::make_iterator_range(out_edges)) {
			auto const target = boost::target(out_edge, *m_graph);
			auto const& execution_instance =
			    m_execution_instance_map.left.at(m_vertex_descriptor_map.left.at(target));
			auto in_edges = boost::in_edges(target, *m_graph);
			std::vector<Input> inputs;
			for (auto const& in_edge : boost::make_iterator_range(in_edges)) {
				auto const source = boost::source(in_edge, *m_graph);
				auto const port_restriction = m_edge_property_map.at(in_edge);
				if (port_restriction) {
					inputs.emplace_back(source, *port_restriction);
				} else {
					inputs.emplace_back(source);
				}
			}
			check_inputs(get_vertex_property(target), execution_instance, inputs);
		}
	} catch (std::runtime_error const& error) {
		// restore old property
		*(m_vertex_property_map.at(vertex_reference)) = std::move(old_vertex);
		throw error;
	}
}

void Graph::update_and_relocate(
    vertex_descriptor const vertex_reference, Vertex&& vertex, std::vector<Input> inputs)
{
	// check inputs work for vertex property
	auto const& execution_instance =
	    m_execution_instance_map.left.at(m_vertex_descriptor_map.left.at(vertex_reference));
	check_inputs(vertex, execution_instance, inputs);
	assert(m_graph);
	// update vertex property
	auto old_vertex = std::move(*(m_vertex_property_map.at(vertex_reference)));
	*(m_vertex_property_map.at(vertex_reference)) = std::move(vertex);
	// remove old edge properties
	auto in_edges = boost::in_edges(vertex_reference, *m_graph);
	for (auto const& in_edge : boost::make_iterator_range(in_edges)) {
		m_edge_property_map.erase(in_edge);
	}
	// check inputs for all targets of vertex support updated vertex proprety
	try {
		auto out_edges = boost::out_edges(vertex_reference, *m_graph);
		for (auto const& out_edge : boost::make_iterator_range(out_edges)) {
			auto const target = boost::target(out_edge, *m_graph);
			auto const& execution_instance =
			    m_execution_instance_map.left.at(m_vertex_descriptor_map.left.at(target));
			auto in_edges = boost::in_edges(target, *m_graph);
			std::vector<Input> inputs;
			for (auto const& in_edge : boost::make_iterator_range(in_edges)) {
				auto const source = boost::source(in_edge, *m_graph);
				auto const port_restriction = m_edge_property_map.at(in_edge);
				if (port_restriction) {
					inputs.emplace_back(source, *port_restriction);
				} else {
					inputs.emplace_back(source);
				}
			}
			check_inputs(get_vertex_property(target), execution_instance, inputs);
		}
	} catch (std::runtime_error const& error) {
		// restore old property
		*(m_vertex_property_map.at(vertex_reference)) = std::move(old_vertex);
		throw error;
	}
	// remove old edges
	boost::remove_in_edge_if(
	    vertex_reference, [](auto const&) { return true; }, *m_graph);
	// add new edges
	add_edges(vertex_reference, execution_instance, inputs);
}

template <typename Archive>
void Graph::save(Archive& ar, std::uint32_t const) const
{
	ar(m_enable_acyclicity_check);
	ar(m_graph);
	ar(m_execution_instance_graph);

	// Manual serialization of m_edge_property_map because edge_descriptor is not invariant under
	// serialization, but its position in boost::edges(m_graph) is.
	std::vector<std::pair<size_t, std::optional<PortRestriction>>> edge_property_list;
	if (m_graph) {
		std::vector<edge_descriptor> edges(
		    boost::edges(*m_graph).first, boost::edges(*m_graph).second);
		for (auto const& p : m_edge_property_map) {
			auto const epos = std::find(edges.begin(), edges.end(), p.first);
			assert(epos != edges.end());
			edge_property_list.push_back({std::distance(edges.begin(), epos), p.second});
		}
		ar(edge_property_list);
	}

	ar(m_vertex_property_map);
	ar(m_vertex_descriptor_map);
	ar(m_execution_instance_map);
}

template <typename Archive>
void Graph::load(Archive& ar, std::uint32_t const)
{
	ar(m_enable_acyclicity_check);
	ar(m_graph);
	ar(m_execution_instance_graph);

	// Manual serialization of m_edge_property_map because edge_descriptor is not invariant under
	// serialization, but its position in boost::edges(m_graph) is.
	std::vector<std::pair<size_t, std::optional<PortRestriction>>> edge_property_list;
	if (m_graph) {
		std::vector<edge_descriptor> edges(
		    boost::edges(*m_graph).first, boost::edges(*m_graph).second);
		ar(edge_property_list);
		for (auto const& p : edge_property_list) {
			m_edge_property_map.insert({edges.at(p.first), p.second});
		}
	}

	ar(m_vertex_property_map);
	ar(m_vertex_descriptor_map);
	ar(m_execution_instance_map);
}

EXPLICIT_INSTANTIATE_CEREAL_LOAD_SAVE(Graph)

} // namespace grenade::vx::signal_flow

CEREAL_CLASS_VERSION(grenade::vx::signal_flow::Graph, 0)
