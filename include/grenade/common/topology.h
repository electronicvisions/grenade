#pragma once
#include "dapr/printable.h"
#include "grenade/common/detail/graph.h"
#include "grenade/common/edge.h"
#include "grenade/common/edge_on_topology.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/graph.h"
#include "grenade/common/vertex.h"
#include "grenade/common/vertex_on_topology.h"
#include "hate/visibility.h"
#include <cereal/access.hpp>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

struct Topology;

extern template struct SYMBOL_VISIBLE Graph<
    Topology,
    detail::BidirectionalMultiGraph,
    Vertex,
    Edge,
    VertexOnTopology,
    EdgeOnTopology,
    std::shared_ptr>;

extern template SYMBOL_VISIBLE std::ostream& operator<<(
    std::ostream& os,
    Graph<
        Topology,
        detail::BidirectionalMultiGraph,
        Vertex,
        Edge,
        VertexOnTopology,
        EdgeOnTopology,
        std::shared_ptr> const& value);


/**
 * Topology of experiment description.
 * It describes the static topology of the experiment's signal-flow/data-flow graph.
 * Populations, projections and digital computations are the graph's vertices, their signal-flow
 * is described in the graph's edges.
 */
struct SYMBOL_VISIBLE GENPYBIND(
    inline_base("*"), holder_type("std::shared_ptr<grenade::common::Topology>")) Topology
    : public Graph<
          Topology,
          detail::BidirectionalMultiGraph,
          Vertex,
          Edge,
          VertexOnTopology,
          EdgeOnTopology,
          std::shared_ptr>
    , public dapr::Printable
{
	/**
	 * Add edge between vertices into network.
	 * @throws std::runtime_error On edge yielding an invalid network
	 * @throws std::runtime_error On edge already present in network
	 * @throws std::out_of_range On source or target of edge not present in network
	 * @param source Source vertex descriptor of edge to add
	 * @param target Target vertex descriptor of edge to add
	 * @param edge Edge property
	 */
	virtual EdgeOnTopology add_edge(
	    VertexOnTopology const& source, VertexOnTopology const& target, Edge const& edge) override;

	/**
	 * Add edge between vertices into network.
	 * @throws std::runtime_error On edge yielding an invalid network
	 * @throws std::runtime_error On edge already present in network
	 * @throws std::out_of_range On source or target of edge not present in network
	 * @param source Source vertex descriptor of edge to add
	 * @param target Target vertex descriptor of edge to add
	 * @param edge Edge property
	 */
	virtual EdgeOnTopology add_edge(
	    VertexOnTopology const& source, VertexOnTopology const& target, Edge&& edge)
	    GENPYBIND(hidden) override;

	/**
	 * Set vertex property.
	 * @param descriptor Vertex descriptor
	 * @param vertex Vertex property
	 * @throws std::out_of_range On vertex not being present in graph
	 * @throws std::runtime_error On vertex property change yielding invalid graph
	 */
	virtual void set(VertexDescriptor const& descriptor, Vertex const& vertex) override;

	/**
	 * Set vertex property.
	 * @param descriptor Vertex descriptor
	 * @param vertex Vertex property
	 * @throws std::out_of_range On vertex not being present in graph
	 * @throws std::runtime_error On vertex property change yielding invalid graph
	 */
	virtual void set(VertexDescriptor const& descriptor, Vertex&& vertex) override
	    GENPYBIND(hidden);

	/**
	 * Set edge property.
	 * @param descriptor Edge descriptor
	 * @param edge Edge property
	 * @throws std::out_of_range On edge not being present in graph
	 * @throws std::runtime_error On edge property change yielding invalid graph
	 */
	virtual void set(EdgeDescriptor const& descriptor, Edge const& edge) override;

	/**
	 * Set edge property.
	 * @param descriptor Edge descriptor
	 * @param edge Edge property
	 * @throws std::out_of_range On edge not being present in graph
	 * @throws std::runtime_error On edge property change yielding invalid graph
	 */
	virtual void set(EdgeDescriptor const& descriptor, Edge&& edge) override GENPYBIND(hidden);

	/**
	 * Check whether strong components of topology are valid.
	 * Ensures, that all strong components vertices feature a homogeneous invariant.
	 */
	bool valid_strong_components() const;

	/**
	 * Check whether topology is valid.
	 * Only valid strong components need to be checked, since all else is checked on adding
	 * elements.
	 */
	virtual bool valid() const override;

	bool operator==(Topology const& other) const;
	bool operator!=(Topology const& other) const;

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	Vertex& get(VertexOnTopology const& descriptor);

private:
	void check_edge(
	    VertexOnTopology const& source,
	    VertexOnTopology const& target,
	    std::optional<EdgeOnTopology> const& edge_descriptor,
	    Vertex const& source_vertex,
	    Vertex const& target_vertex,
	    Edge const& edge) const;

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);


public:
	using Graph::get;
};

} // namespace common
} // namespace grenade

CEREAL_SPECIALIZE_FOR_ALL_ARCHIVES(
    grenade::common::Topology, cereal::specialization::member_serialize)
