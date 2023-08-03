#include "grenade/vx/signal_flow/graph.h"

#include "grenade/cerealization.h"
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>

namespace cereal {

template <class Archive>
void CEREAL_SAVE_FUNCTION_NAME(
    Archive& ar, grenade::vx::signal_flow::Graph::execution_instance_map_type const& data)
{
	std::vector<std::pair<
	    grenade::vx::signal_flow::Graph::vertex_descriptor,
	    grenade::vx::common::ExecutionInstanceID>>
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
	    grenade::vx::common::ExecutionInstanceID>>
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
		[[maybe_unused]] auto const [_, inserted] =
		    boost::add_edge(vertices.at(u), vertices.at(v), data);
		assert(inserted);
	}
}

} // namespace cereal

namespace grenade::vx::signal_flow {

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
