#include "grenade/vx/graph.h"

#include <sstream>
#include <log4cxx/logger.h>
#include "grenade/vx/execution_instance.h"
#include "halco/hicann-dls/vx/chip.h"
#include "hate/timer.h"
#include "hate/type_list.h"
#include "hate/type_traits.h"

namespace grenade::vx {

Graph::Graph() : m_graph(), m_logger(log4cxx::Logger::getLogger("grenade.Graph")) {}

Graph::vertex_descriptor Graph::add(
    Vertex const& vertex,
    coordinate::ExecutionInstance const& execution_instance,
    std::vector<vertex_descriptor> inputs)
{
	hate::Timer const timer;

	auto const check_inputs = [&](auto const& v) {
		size_t const num_inputs = v.inputs().size();

		if (num_inputs != inputs.size()) {
			std::stringstream ss;
			ss << "Number of supplied inputs (" << inputs.size() << ") does not match expected ("
			   << num_inputs << ") from vertex.";
			throw std::runtime_error(ss.str());
		}

		for (size_t input_index = 0; input_index < inputs.size(); ++input_index) {
			auto const check_single_input = [&](auto const& w) {
				// check incoming port matches positional input port
				if (v.inputs().at(input_index) != w.output()) {
					std::stringstream ss;
					ss << "Vertex port " << v.inputs().at(input_index) << " at input index ("
					   << input_index << ") does not match provided port " << w.output()
					   << " of input vertex.";
					throw std::runtime_error(ss.str());
				}

				if (single_outgoing_vertex(w)) {
					if (boost::out_degree(inputs.at(input_index), m_graph) != 0) {
						throw std::runtime_error(
						    "Input vertex port does not allow more than one edge.");
					}
				}

				// check connection goes forward in time
				auto const& execution_instance_property_map =
				    boost::get(ExecutionInstancePropertyTag(), m_graph);
				auto const& out_execution_instance =
				    execution_instance_property_map[inputs.at(input_index)];
				auto const connection_type = w.output().type;
				auto const connection_type_can_connect =
				    std::find(
				        can_connect_different_execution_instances.begin(),
				        can_connect_different_execution_instances.end(),
				        connection_type) != std::cend(can_connect_different_execution_instances);
				if (connection_type_can_connect && v.can_connect_different_execution_instances &&
				    w.can_connect_different_execution_instances) {
					if (!(execution_instance > out_execution_instance)) {
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

			auto const& vertex_property_map = boost::get(VertexPropertyTag(), m_graph);
			std::visit(check_single_input, vertex_property_map[inputs.at(input_index)]);
		}
	};

	std::visit(check_inputs, vertex);

	auto const descriptor = boost::add_vertex(m_graph);
	boost::put(VertexPropertyTag(), m_graph, descriptor, vertex);
	boost::put(ExecutionInstancePropertyTag(), m_graph, descriptor, execution_instance);

	for (auto const input : inputs) {
		auto const [_, success] = boost::add_edge(input, descriptor, m_graph);
		if (!success) {
			throw std::logic_error("Adding edge to graph unsuccessful.");
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

Graph::ordered_vertices_type Graph::get_ordered_vertices() const
{
	hate::Timer const timer;
	ordered_vertices_type ordered_vertices;
	auto const& execution_instance_map = boost::get(ExecutionInstancePropertyTag(), m_graph);
	for (auto const vertex : boost::make_iterator_range(boost::vertices(m_graph))) {
		auto const execution_instance = execution_instance_map[vertex];
		auto const execution_index = execution_instance.toExecutionIndex();
		auto const dls_global = execution_instance.toHemisphereGlobal().toDLSGlobal();
		auto const hemisphere = execution_instance.toHemisphereGlobal().toHemisphereOnDLS();
		ordered_vertices[execution_index][dls_global][hemisphere].push_back(
		    vertex_descriptor(vertex));
	}
	LOG4CXX_TRACE(
	    m_logger,
	    "get_ordered_vertices(): Generated ordered vertex map in " << timer.print() << ".");
	return ordered_vertices;
}

} // namespace grenade::vx
