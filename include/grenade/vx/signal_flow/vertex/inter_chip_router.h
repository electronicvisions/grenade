#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/time_domain_on_topology.h"
#include "hate/visibility.h"
#include <optional>

namespace grenade::vx::signal_flow {
namespace vertex {

/**
 * Global inter chip router.
 * Connects output routing tables to input routing tables.
 */
struct InterChipRouter : public grenade::common::PartitionedVertex
{
	InterChipRouter(
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor)
	    SYMBOL_VISIBLE;

	virtual bool valid_edge_from(
	    Vertex const& source, grenade::common::Edge const& edge) const override;

	virtual bool valid_edge_to(
	    Vertex const& target, grenade::common::Edge const& edge) const override;

	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const override;
	virtual std::vector<grenade::common::Vertex::Port> get_output_ports() const override;

	virtual std::optional<grenade::common::TimeDomainOnTopology> get_time_domain() const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(Vertex const& other) const override;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);

	grenade::common::TimeDomainOnTopology m_time_domain;
};

} // namespace vertex
} // namespace grenade::vx::signal_flow