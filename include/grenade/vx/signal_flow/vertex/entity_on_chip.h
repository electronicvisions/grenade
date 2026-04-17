#pragma once
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <iosfwd>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow {
namespace vertex GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * Entity on chip carrying chip on connection information.
 */
struct SYMBOL_VISIBLE EntityOnChip : public grenade::common::PartitionedVertex
{
	/**
	 * Construct entity on chip.
	 * @param chip_on_connection Coordinate of chip to use
	 * @param time_domain Time domain to use
	 * @param execution_instance_on_executor Execution instance to use
	 */
	EntityOnChip(
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor)
	    SYMBOL_VISIBLE;

	common::ChipOnConnection chip_on_connection;

	virtual std::optional<grenade::common::TimeDomainOnTopology> get_time_domain() const override;

protected:
	virtual bool is_equal_to(grenade::common::Vertex const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);

	grenade::common::TimeDomainOnTopology m_time_domain;
};

} // namespace vertex
} // namespace grenade::vx::signal_flow
