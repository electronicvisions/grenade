#pragma once
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/vertex.h"
#include <memory>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * A temporally and spacially partitioned vertex.
 * Each vertex is assigned to exactly one single execution instance on the executor.
 */
struct SYMBOL_VISIBLE GENPYBIND(holder_type("std::shared_ptr<grenade::common::PartitionedVertex>"))
    PartitionedVertex : public Vertex
{
	/**
	 * For a partitioned vertex, all vertices in a strong component are required to
	 * be in the same execution instance on the executor.
	 */
	struct StrongComponentInvariant : public Vertex::StrongComponentInvariant
	{
		StrongComponentInvariant(
		    std::optional<ExecutionInstanceOnExecutor> execution_instance_on_executor);

		std::optional<ExecutionInstanceOnExecutor> execution_instance_on_executor;

		std::unique_ptr<Vertex::StrongComponentInvariant> copy() const;
		std::unique_ptr<Vertex::StrongComponentInvariant> move();

	protected:
		virtual std::ostream& print(std::ostream& os) const;
		virtual bool is_equal_to(Vertex::StrongComponentInvariant const& other) const;
	};

	/**
	 * Construct partitioned vertex with optional execution instance on executor.
	 * @param execution_instance_on_executor Optional execution instance on executor
	 */
	PartitionedVertex(
	    std::optional<ExecutionInstanceOnExecutor> const& execution_instance_on_executor =
	        std::nullopt);

	/**
	 * Get execution instance on executor.
	 */
	std::optional<ExecutionInstanceOnExecutor> const& get_execution_instance_on_executor() const;

	/**
	 * Set execution instance on executor.
	 * If a derived class is only conditionally partitionable, this method can be overwritten to
	 * throw an error if the vertex is not partitionable, but a non-null value is given.
	 */
	virtual void set_execution_instance_on_executor(
	    std::optional<ExecutionInstanceOnExecutor> const& value);

	/**
	 * Get invariant of vertex which is required to be equal across its strong component.
	 */
	virtual std::unique_ptr<Vertex::StrongComponentInvariant> get_strong_component_invariant()
	    const override;

	/**
	 * Get whether the edge from the source vertex to this vertex is valid.
	 * This is the case if the execution instance transition matches the expectation when the source
	 * vertex is a partitioned vertex (in addition to the common requirements of the Vertex).
	 * @param source Source vertex
	 * @param edge Edge
	 */
	virtual bool valid_edge_from(Vertex const& source, Edge const& edge) const override;

	/**
	 * Get whether the edge to the target vertex from this vertex is valid.
	 * This is the case if the execution instance transition matches the expectation when the target
	 * vertex is a partitioned vertex (in addition to the common requirements of the Vertex).
	 * @param target Target vertex
	 * @param edge Edge
	 */
	virtual bool valid_edge_to(Vertex const& target, Edge const& edge) const override;

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(Vertex const& other) const override;

	std::optional<ExecutionInstanceOnExecutor> m_execution_instance_on_executor;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace common
} // namespace grenade
