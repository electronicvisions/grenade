#include "grenade/vx/signal_flow/vertex/padi_bus.h"

#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

PADIBus::PADIBus(
    Coordinate const& coordinate,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    coordinate(coordinate)
{}

std::vector<grenade::common::Vertex::Port> PADIBus::get_input_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::CrossbarOutputLabel),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> PADIBus::get_output_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::SynapseDriverInputLabel),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

bool PADIBus::valid_edge_from(Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}
	if (auto const crossbar_node = dynamic_cast<CrossbarNode const*>(&source); crossbar_node) {
		return crossbar_node->coordinate.toCrossbarOutputOnDLS() ==
		       coordinate.toCrossbarOutputOnDLS();
	}
	return false;
}

std::unique_ptr<grenade::common::Vertex> PADIBus::copy() const
{
	return std::make_unique<PADIBus>(*this);
}

std::unique_ptr<grenade::common::Vertex> PADIBus::move()
{
	return std::make_unique<PADIBus>(std::move(*this));
}

std::ostream& PADIBus::print(std::ostream& os) const
{
	os << "PADIBus(coordinate: " << coordinate << ")";
	return os;
}

bool PADIBus::is_equal_to(Vertex const& other) const
{
	auto const& other_bus = static_cast<PADIBus const&>(other);
	return coordinate == other_bus.coordinate && EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
