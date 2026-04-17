#include "grenade/vx/signal_flow/vertex/pad_readout.h"

#include "grenade/common/edge.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

PadReadoutView::Parameterization::Parameterization() {}

std::unique_ptr<grenade::common::PortData> PadReadoutView::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> PadReadoutView::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

std::ostream& PadReadoutView::Parameterization::print(std::ostream& os) const
{
	return os << "Parameterization(enable_buffered: " << enable_buffered << ")";
}

bool PadReadoutView::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	auto const& other_parameterization = static_cast<Parameterization const&>(other);
	return enable_buffered == other_parameterization.enable_buffered;
}


PadReadoutView::PadReadoutView(
    Source const& source,
    Coordinate const& coordinate,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    source(source),
    coordinate(coordinate)
{
}

bool PadReadoutView::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& port_data) const
{
	return input_port_on_vertex == 1 &&
	       dynamic_cast<Parameterization const*>(&port_data) != nullptr;
}

std::vector<grenade::common::Vertex::Port> PadReadoutView::get_input_ports() const
{
	return {
	    grenade::common::Vertex::Port(
	        VertexPortType(ConnectionType::MembraneVoltage),
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

std::vector<grenade::common::Vertex::Port> PadReadoutView::get_output_ports() const
{
	return {};
}

bool PadReadoutView::valid_edge_from(Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}
	if (auto const neuron_ptr = dynamic_cast<NeuronView const*>(&source); neuron_ptr) {
		auto const channels_on_source = edge.get_channels_on_source().get_elements();
		if (halco::hicann_dls::vx::v3::AtomicNeuronOnDLS(
		        neuron_ptr->get_columns().at(channels_on_source.at(0).value.at(0)),
		        neuron_ptr->row) != this->source) {
			return false;
		}
		return true;
	}
	return false;
}

std::unique_ptr<grenade::common::Vertex> PadReadoutView::copy() const
{
	return std::make_unique<PadReadoutView>(*this);
}

std::unique_ptr<grenade::common::Vertex> PadReadoutView::move()
{
	return std::make_unique<PadReadoutView>(std::move(*this));
}

bool PadReadoutView::is_equal_to(Vertex const& other) const
{
	auto const& other_pad = static_cast<PadReadoutView const&>(other);
	return (source == other_pad.source) && (coordinate == other_pad.coordinate) &&
	       EntityOnChip::is_equal_to(other);
}

std::ostream& PadReadoutView::print(std::ostream& os) const
{
	return os << "PadReadoutView(source: " << source << " coord: " << coordinate << ")";
}

} // namespace grenade::vx::signal_flow::vertex
