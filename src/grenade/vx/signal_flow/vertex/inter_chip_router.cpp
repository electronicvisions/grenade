#include "grenade/vx/signal_flow/vertex/inter_chip_router.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"
#include "grenade/vx/signal_flow/vertex/input_routing_table.h"
#include "grenade/vx/signal_flow/vertex/output_routing_table.h"

namespace grenade::vx::signal_flow {
namespace vertex {

InterChipRouter::InterChipRouter(
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    PartitionedVertex(execution_instance_on_executor), m_time_domain(time_domain)
{
}

bool InterChipRouter::valid_edge_from(Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}

	// Source needs to be an output from the crossbar on the same chip the FPGA belongs to.
	return dynamic_cast<OutputRoutingTable const*>(&source) != nullptr;
}

bool InterChipRouter::valid_edge_to(Vertex const& target, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_to(target, edge)) {
		return false;
	}

	// Target needs to be an input to the crossbar.
	return dynamic_cast<InputRoutingTable const*>(&target) != nullptr;
}

std::vector<grenade::common::Vertex::Port> InterChipRouter::get_input_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::TimedSpikeFromChipSequence),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}
std::vector<grenade::common::Vertex::Port> InterChipRouter::get_output_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::TimedSpikeToChipSequence),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::optional<grenade::common::TimeDomainOnTopology> InterChipRouter::get_time_domain() const
{
	return m_time_domain;
}

std::unique_ptr<grenade::common::Vertex> InterChipRouter::copy() const
{
	return std::make_unique<InterChipRouter>(*this);
}
std::unique_ptr<grenade::common::Vertex> InterChipRouter::move()
{
	return std::make_unique<InterChipRouter>(std::move(*this));
}

std::ostream& InterChipRouter::print(std::ostream& os) const
{
	os << "InterChipRouter()";
	return os;
}

bool InterChipRouter::is_equal_to(Vertex const&) const
{
	return true;
}

} // namespace vertex
} // namespace grenade::vx::signal_flow
