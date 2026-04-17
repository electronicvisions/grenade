#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"

#include <ostream>

namespace grenade::vx::signal_flow::vertex {

EntityOnChip::EntityOnChip(
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    PartitionedVertex(execution_instance_on_executor),
    chip_on_connection(chip_on_connection),
    m_time_domain(time_domain)
{}

std::optional<grenade::common::TimeDomainOnTopology> EntityOnChip::get_time_domain() const
{
	return m_time_domain;
}

bool EntityOnChip::is_equal_to(Vertex const& other) const
{
	return chip_on_connection == static_cast<EntityOnChip const&>(other).chip_on_connection &&
	       m_time_domain == static_cast<EntityOnChip const&>(other).m_time_domain;
}

std::ostream& EntityOnChip::print(std::ostream& os) const
{
	os << "EntityOnChip(";
	PartitionedVertex::print(os) << ", " << chip_on_connection << ", " << m_time_domain << ")";
	return os;
}

} // namespace grenade::vx::signal_flow::vertex
