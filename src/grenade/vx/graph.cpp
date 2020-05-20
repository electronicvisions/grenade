#include "grenade/vx/graph.h"

#include <iterator>
#include <sstream>
#include <boost/graph/exception.hpp>
#include <boost/graph/topological_sort.hpp>
#include <log4cxx/logger.h>
#include "grenade/vx/connection_restriction.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/input.h"
#include "halco/hicann-dls/vx/chip.h"
#include "hate/timer.h"
#include "hate/type_list.h"
#include "hate/type_traits.h"

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
    Vertex const& vertex,
    coordinate::ExecutionInstance const& execution_instance,
    std::vector<Input> inputs)
{
	hate::Timer const timer;

	auto const check_inputs = [&](auto const& v) {
		size_t const num_inputs = v.inputs().size();

		if constexpr (v.variadic_input) {
			if (num_inputs > inputs.size()) {
				std::stringstream ss;
				ss << "Number of supplied inputs (" << inputs.size()
				   << ") does not match expected minimal (" << num_inputs << ") from vertex.";
				throw std::runtime_error(ss.str());
			}
		} else {
			if (num_inputs != inputs.size()) {
				std::stringstream ss;
				ss << "Number of supplied inputs (" << inputs.size()
				   << ") does not match expected (" << num_inputs << ") from vertex.";
				throw std::runtime_error(ss.str());
			}
		}

		for (size_t i = 0; i < inputs.size(); ++i) {
			auto const& input = inputs.at(i);
			size_t const input_index = std::min(i, num_inputs - 1);

			auto const check_single_input = [&](auto const& w) {
				if (input.port_restriction) {
					// check incoming port restriction is valid
					if (!input.port_restriction->is_restriction_of(w.output())) {
						std::stringstream ss;
						ss << *input.port_restriction << " at input index (" << i
						   << ") is not a restriction of provided port " << w.output()
						   << " of input vertex.";
						throw std::runtime_error(ss.str());
					}

					// check incoming restricted port matches positional input port
					if ((v.inputs().at(input_index).type != w.output().type) ||
					    (v.inputs().at(input_index).size != input.port_restriction->size())) {
						std::stringstream ss;
						ss << "Vertex port " << v.inputs().at(input_index) << " at input index ("
						   << i << ") does not match provided port " << w.output() << " with "
						   << *input.port_restriction << " of input vertex.";
						throw std::runtime_error(ss.str());
					}
				} else {
					// check incoming port matches positional input port
					if (v.inputs().at(input_index) != w.output()) {
						std::stringstream ss;
						ss << "Vertex port " << v.inputs().at(input_index) << " at input index ("
						   << i << ") does not match provided port " << w.output()
						   << " of input vertex.";
						throw std::runtime_error(ss.str());
					}
				}

				if (!supports_input_from(v, w, input.port_restriction)) {
					throw std::runtime_error("Vertex does not support input from incoming vertex.");
				}

				// check connection goes forward in time
				auto const& out_execution_instance = m_execution_instance_map.left.at(
				    m_vertex_descriptor_map.left.at(input.descriptor));
				auto const connection_type = w.output().type;
				auto const connection_type_can_connect =
				    std::find(
				        can_connect_different_execution_instances.begin(),
				        can_connect_different_execution_instances.end(),
				        connection_type) != std::cend(can_connect_different_execution_instances);
				if (connection_type_can_connect && v.can_connect_different_execution_instances &&
				    w.can_connect_different_execution_instances) {
					if (execution_instance == out_execution_instance) {
						throw std::runtime_error(
						    "Connection does not go forward in time dimension.");
					}
				} else {
					if (execution_instance != out_execution_instance) {
						throw std::runtime_error(
						    "Connection between specified vertices not allowed "
						    "over different times.");
					}
				}
			};

			std::visit(check_single_input, m_vertex_property_map.at(input.descriptor));
		}
		LOG4CXX_TRACE(m_logger, "add(): Adding vertex " << v << ".");
	};

	std::visit(check_inputs, vertex);

	auto const descriptor = boost::add_vertex(m_graph);
	{
		auto const [_, success] = m_vertex_property_map.emplace(descriptor, vertex);
		if (!success) {
			throw std::logic_error("Adding vertex property graph unsuccessful.");
		}
	}

	vertex_descriptor execution_instance_descriptor;
	auto const execution_instance_map_it = m_execution_instance_map.right.find(execution_instance);
	if (execution_instance_map_it == m_execution_instance_map.right.end()) {
		execution_instance_descriptor = boost::add_vertex(m_execution_instance_graph);
		m_execution_instance_map.insert({execution_instance_descriptor, execution_instance});
	} else {
		execution_instance_descriptor = execution_instance_map_it->second;
	}
	m_vertex_descriptor_map.insert({descriptor, execution_instance_descriptor});

	bool new_edges_across_execution_instances = false;
	for (auto const& input : inputs) {
		// add edge to vertex graph
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

	if (m_enable_acyclicity_check && new_edges_across_execution_instances) {
		if (!is_acyclic_execution_instance_graph()) {
			throw std::runtime_error("Execution instance graph cyclic.");
		}
	}

	LOG4CXX_TRACE(
	    m_logger, "add(): Added vertex " << descriptor << " at " << execution_instance << " in "
	                                     << timer.print() << ".");
	return descriptor;
}

Graph::graph_type const& Graph::get_graph() const
{
	return m_graph;
}

Graph::graph_type const& Graph::get_execution_instance_graph() const
{
	return m_execution_instance_graph;
}

Graph::vertex_property_map_type const& Graph::get_vertex_property_map() const
{
	return m_vertex_property_map;
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

template <typename T>
class NullOutputIterator : public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
	NullOutputIterator& operator=(const T&)
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

} // namespace grenade::vx
