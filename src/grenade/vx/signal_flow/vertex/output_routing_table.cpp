#include "grenade/vx/signal_flow/vertex/output_routing_table.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"
#include "grenade/vx/signal_flow/vertex/inter_chip_router.h"
#include "hate/indent.h"

namespace grenade::vx::signal_flow {
namespace vertex {

OutputRoutingTable::Parameterization::Parameterization(
    std::map<
        halco::hicann_dls::vx::v3::OutputRoutingTableEntryOnFPGA,
        haldls::vx::v3::OutputRoutingTableEntry::Label> table_entries) :
    entries(std::move(table_entries))
{
}

std::unique_ptr<grenade::common::PortData> OutputRoutingTable::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> OutputRoutingTable::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

std::ostream& OutputRoutingTable::Parameterization::print(std::ostream& os) const
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

bool OutputRoutingTable::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	auto other_cast = static_cast<Parameterization const&>(other);
	return entries == other_cast.entries;
}


OutputRoutingTable::OutputRoutingTable(
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor)
{
}

bool OutputRoutingTable::valid_edge_from(
    Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}

	// Source needs to be an output from the crossbar on the same chip the FPGA belongs to.
	if (auto* crossbar_output = dynamic_cast<CrossbarL2Output const*>(&source)) {
		return chip_on_connection == crossbar_output->chip_on_connection;
	}
	return false;
}

bool OutputRoutingTable::valid_edge_to(
    Vertex const& target, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_to(target, edge)) {
		return false;
	}

	// Target needs to be an input to the crossbar.
	return dynamic_cast<InterChipRouter const*>(&target) != nullptr;
}

std::vector<grenade::common::Vertex::Port> OutputRoutingTable::get_input_ports() const
{
	return {
	    Port(
	        VertexPortType(ConnectionType::TimedSpikeFromChipSequence),
	        grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})})),
	    grenade::common::Vertex::Port(
	        ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}
std::vector<grenade::common::Vertex::Port> OutputRoutingTable::get_output_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::TimedSpikeFromChipSequence),
	    grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::unique_ptr<grenade::common::Vertex> OutputRoutingTable::copy() const
{
	return std::make_unique<OutputRoutingTable>(*this);
}
std::unique_ptr<grenade::common::Vertex> OutputRoutingTable::move()
{
	return std::make_unique<OutputRoutingTable>(std::move(*this));
}

std::ostream& OutputRoutingTable::print(std::ostream& os) const
{
	os << "OutputRoutingTable()";
	return os;
}

bool OutputRoutingTable::is_equal_to(Vertex const&) const
{
	return true;
}


} // namespace vertex
} // namespace grenade::vx::signal_flow
