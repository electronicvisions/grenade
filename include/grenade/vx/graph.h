#pragma once
#include <cstddef>
#include <map>
#include <unordered_map>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
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

/**
 * Placed computation graph.
 *
 * A vertex represent a unit which processes data.
 * An edge represents the data flow.
 *
 * Vertices are physically and temporally placed on a specific ExecutionInstance.
 * Data flow between different execution instances is restricted to off-chip data.
 * Access to execution instance subgraphs and mapping between execution instance dependency graph
 * and the complete data-flow graph is provided.
 *
 * The dependence graph of execution instances has to be acyclic in order to be executable, while
 * a subgraph tied to a execution instance can partly be cyclic, e.g. via recurrent routing of
 * on-chip events. This is enforced by checks on addition of vertices/edges.
 */
class Graph
{
public:
	/** Bidirectional graph without parallel edges. */
	typedef boost::adjacency_list<
	    boost::vecS,
	    boost::vecS,
	    boost::bidirectionalS,
	    boost::no_property,
	    boost::no_property>
	    graph_type;

	typedef graph_type::vertex_descriptor vertex_descriptor;

	typedef std::unordered_map<vertex_descriptor, Vertex> vertex_property_map_type;
	typedef boost::
	    bimap<vertex_descriptor, boost::bimaps::unordered_set_of<coordinate::ExecutionInstance>>
	        execution_instance_map_type;
	// TODO: maybe make vertex descriptors of the two graphs unique?
	typedef boost::bimap<
	    boost::bimaps::set_of<vertex_descriptor>,
	    boost::bimaps::multiset_of<vertex_descriptor>>
	    vertex_descriptor_map_type;

	/**
	 * Construct graph.
	 * @param enable_acyclicity_check Enable check for acyclicity in execution instance graph on
	 * every add call where a connection between previously unconnected execution instances is made.
	 */
	Graph(bool enable_acyclicity_check = true) SYMBOL_VISIBLE;

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
	 * Get constant reference to underlying graph of execution instances.
	 * @return Constant reference to underlying graph of execution instances
	 */
	graph_type const& get_execution_instance_graph() const SYMBOL_VISIBLE;

	/**
	 * Get constant reference to vertex property map.
	 * @return Constant reference to vertex property map
	 */
	vertex_property_map_type const& get_vertex_property_map() const SYMBOL_VISIBLE;

	/**
	 * Get constant reference to vertex property map.
	 * @return Constant reference to vertex property map
	 */
	execution_instance_map_type const& get_execution_instance_map() const SYMBOL_VISIBLE;

	/**
	 * Get constant reference to vertex descriptor map.
	 * @return Constant reference to vertex descriptor map
	 */
	vertex_descriptor_map_type const& get_vertex_descriptor_map() const SYMBOL_VISIBLE;

	typedef std::map<
	    coordinate::ExecutionIndex,
	    std::map<halco::hicann_dls::vx::DLSGlobal, std::vector<Graph::vertex_descriptor>>>
	    ordered_vertices_type;

	/**
	 * Get whether the underlying execution instance graph is acyclic.
	 * This is a necessary requirement for executability.
	 * @return Boolean value
	 */
	bool is_acyclic_execution_instance_graph() const;

private:
	bool m_enable_acyclicity_check;
	graph_type m_graph;
	graph_type m_execution_instance_graph;
	vertex_property_map_type m_vertex_property_map;
	vertex_descriptor_map_type m_vertex_descriptor_map;
	execution_instance_map_type m_execution_instance_map;
	log4cxx::Logger* m_logger;
};

} // namespace grenade::vx
