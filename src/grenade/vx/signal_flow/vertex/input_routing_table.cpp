#include "grenade/vx/signal_flow/vertex/input_routing_table.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"
#include "grenade/vx/signal_flow/vertex/inter_chip_router.h"
#include "hate/indent.h"

namespace grenade::vx::signal_flow {
namespace vertex {

InputRoutingTable::Parameterization::Parameterization(
    std::map<
        halco::hicann_dls::vx::v3::InputRoutingTableEntryOnFPGA,
        haldls::vx::v3::InputRoutingTableEntry::Label> table_entries) :
    entries(std::move(table_entries))
{
}

std::unique_ptr<grenade::common::PortData> InputRoutingTable::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> InputRoutingTable::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

std::ostream& InputRoutingTable::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n" << hate::Indentation("\t");
	for (auto const& [key, value] : entries) {
		ios << key << ": " << value << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool InputRoutingTable::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	auto other_cast = static_cast<Parameterization const&>(other);
	return entries == other_cast.entries;
}

InputRoutingTable::InputRoutingTable(
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor)
{
}

bool InputRoutingTable::valid_edge_from(
    Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}

	// Source needs to be an output from the crossbar on the same chip the FPGA belongs to.
	return dynamic_cast<InterChipRouter const*>(&source) != nullptr;
}

bool InputRoutingTable::valid_edge_to(Vertex const& target, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_to(target, edge)) {
		return false;
	}

	// Target needs to be an input to the crossbar.
	if (auto* crossbar_input = dynamic_cast<CrossbarL2Input const*>(&target)) {
		return chip_on_connection == crossbar_input->chip_on_connection;
	}
	return false;
}

std::vector<grenade::common::Vertex::Port> InputRoutingTable::get_input_ports() const
{
	return {
	    Port(
	        VertexPortType(ConnectionType::TimedSpikeToChipSequence),
	        grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})})),
	    grenade::common::Vertex::Port(
	        ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}
std::vector<grenade::common::Vertex::Port> InputRoutingTable::get_output_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::TimedSpikeToChipSequence),
	    grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::unique_ptr<grenade::common::Vertex> InputRoutingTable::copy() const
{
	return std::make_unique<InputRoutingTable>(*this);
}
std::unique_ptr<grenade::common::Vertex> InputRoutingTable::move()
{
	return std::make_unique<InputRoutingTable>(std::move(*this));
}

std::ostream& InputRoutingTable::print(std::ostream& os) const
{
	os << "InputRoutingTable()";
	return os;
}

bool InputRoutingTable::is_equal_to(Vertex const&) const
{
	return true;
}


} // namespace vertex
} // namespace grenade::vx::signal_flow
