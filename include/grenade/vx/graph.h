#pragma once
#include <cstddef>
#include <map>
#include <unordered_map>
#include <boost/graph/adjacency_list.hpp>

#include "grenade/vx/execution_instance.h"
#include "grenade/vx/vertex.h"
#include "halco/common/typed_array.h"
#include "hate/visibility.h"

namespace log4cxx {
class Logger;
} // namespace log4cxx

namespace halco::hicann_dls::vx {
struct DLSGlobal;
struct HemisphereOnDLS;
} // namespace halco::hicann_dls::vx

namespace grenade::vx {

/** Placed computation graph. */
class Graph
{
public:
	/** Directed graph without parallel edges. */
	typedef boost::adjacency_list<
	    boost::vecS,
	    boost::vecS,
	    boost::bidirectionalS,
	    boost::no_property,
	    boost::no_property>
	    graph_type;

	typedef graph_type::vertex_descriptor vertex_descriptor;

	struct VertexProperty
	{
		coordinate::ExecutionInstance execution_instance;
		Vertex vertex;
	};

	typedef std::unordered_map<vertex_descriptor, VertexProperty> vertex_property_map_type;

	/** Default constructor. */
	Graph() SYMBOL_VISIBLE;

	/**
	 * Add vertex on specified execution instance with specified inputs.
	 * No checks are performed against whether the section of specified chip instance is already
	 * used.
	 * TODO: Do we want this and thereby have a separate resource manager?
	 * TODO: We might want to have a more fancy vertex descriptor return type
	 * Perform checks for:
	 *  - connection between vertex types is allowed
	 *  - vertex inputs match provided input descriptors output
	 *  - connection does not lead to acyclicity
	 *  - connection goes forward in time
	 * @param vertex Vertex configuration
	 * @param execution_instance Execution instance to place on
	 * @param inputs Positional list input vertex descriptors
	 * @return Vertex descriptor of added vertex
	 */
	vertex_descriptor add(
	    Vertex const& vertex,
	    coordinate::ExecutionInstance const& execution_instance,
	    std::vector<vertex_descriptor> inputs) SYMBOL_VISIBLE;

	/**
	 * Get constant reference to underlying graph.
	 * @return Constant reference to underlying graph
	 */
	graph_type const& get_graph() const SYMBOL_VISIBLE;

	/**
	 * Get constant reference to vertex property map.
	 * @return Constant reference to vertex property map
	 */
	vertex_property_map_type const& get_vertex_property_map() const SYMBOL_VISIBLE;

	typedef std::map<
	    coordinate::ExecutionIndex,
	    std::map<halco::hicann_dls::vx::DLSGlobal, std::vector<Graph::vertex_descriptor>>>
	    ordered_vertices_type;

	/**
	 * Get two dimensional map of vertex lists with physical and temporal dimension.
	 * @return Map of vertex lists
	 */
	ordered_vertices_type get_ordered_vertices() const;

private:
	graph_type m_graph;
	vertex_property_map_type m_vertex_property_map;
	log4cxx::Logger* m_logger;
};

} // namespace grenade::vx
