#include "grenade/vx/signal_flow/vertex/synapse_driver.h"

#include "grenade/common/edge.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "grenade/vx/signal_flow/vertex/padi_bus.h"
#include "halco/common/iter_all.h"
#include "hate/indent.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

SynapseDriver::Parameterization::Parameterization() {}

std::unique_ptr<grenade::common::PortData> SynapseDriver::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> SynapseDriver::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

std::ostream& SynapseDriver::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	ios << hate::Indentation("\t");
	ios << row_address_compare_mask << "\n";
	for (auto const coord :
	     halco::common::iter_all<halco::hicann_dls::vx::v3::SynapseRowOnSynapseDriver>()) {
		ios << coord << ": " << row_modes[coord] << "\n";
	}
	ios << "enable_address_out: " << enable_address_out << "\n";
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool SynapseDriver::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	auto const& other_parameterization = static_cast<Parameterization const&>(other);
	return row_address_compare_mask == other_parameterization.row_address_compare_mask &&
	       row_modes == other_parameterization.row_modes;
}


SynapseDriver::SynapseDriver(
    Coordinate const& coordinate,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    coordinate(coordinate)
{}

bool SynapseDriver::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	return input_port_on_vertex == 1 && dynamic_cast<Parameterization const*>(&data) != nullptr;
}

std::vector<grenade::common::Vertex::Port> SynapseDriver::get_input_ports() const
{
	return {
	    grenade::common::Vertex::Port(
	        VertexPortType(ConnectionType::SynapseDriverInputLabel),
	        grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})})),
	    grenade::common::Vertex::Port(
	        ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> SynapseDriver::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    VertexPortType(ConnectionType::SynapseInputLabel),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence(
	        {grenade::common::MultiIndex({0}), grenade::common::MultiIndex({1})}))};
}


bool SynapseDriver::valid_edge_from(Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}
	if (auto const padi_bus = dynamic_cast<PADIBus const*>(&source); padi_bus) {
		auto const channels_on_source = edge.get_channels_on_source().get_elements();
		if (halco::hicann_dls::vx::v3::PADIBusOnDLS(
		        coordinate.toSynapseDriverOnSynapseDriverBlock().toPADIBusOnPADIBusBlock(),
		        coordinate.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS()) !=
		    padi_bus->coordinate) {
			return false;
		}
		return true;
	}
	return false;
}

std::unique_ptr<grenade::common::Vertex> SynapseDriver::copy() const
{
	return std::make_unique<SynapseDriver>(*this);
}

std::unique_ptr<grenade::common::Vertex> SynapseDriver::move()
{
	return std::make_unique<SynapseDriver>(std::move(*this));
}

std::ostream& SynapseDriver::print(std::ostream& os) const
{
	return os << "SynapseDriver(coordinate: " << coordinate << ")";
}

bool SynapseDriver::is_equal_to(Vertex const& other) const
{
	auto const& other_syndrv = static_cast<SynapseDriver const&>(other);
	return (coordinate == other_syndrv.coordinate) && EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
