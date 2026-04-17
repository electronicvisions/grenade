#include "grenade/vx/signal_flow/vertex/background_spike_source.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

BackgroundSpikeSource::Parameterization::Parameterization(
    haldls::vx::BackgroundSpikeSource config) :
    config(config)
{
}

std::unique_ptr<grenade::common::PortData> BackgroundSpikeSource::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> BackgroundSpikeSource::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

bool BackgroundSpikeSource::Parameterization::is_equal_to(PortData const& other) const
{
	return config == static_cast<Parameterization const&>(other).config;
}

std::ostream& BackgroundSpikeSource::Parameterization::print(std::ostream& os) const
{
	return os << "Parameterization(" << config << ")";
}


BackgroundSpikeSource::BackgroundSpikeSource(
    Coordinate const& coordinate,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    coordinate(coordinate)
{}

bool BackgroundSpikeSource::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	return input_port_on_vertex == 0 && dynamic_cast<Parameterization const*>(&data) != nullptr;
}

bool BackgroundSpikeSource::valid_edge_to(
    Vertex const& target, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_to(target, edge)) {
		return false;
	}
	if (auto const crossbar_node = dynamic_cast<CrossbarNode const*>(&target); crossbar_node) {
		return crossbar_node->coordinate.toCrossbarInputOnDLS() ==
		       coordinate.toCrossbarInputOnDLS();
	}
	return false;
}

std::vector<grenade::common::Vertex::Port> BackgroundSpikeSource::get_input_ports() const
{
	return {grenade::common::Vertex::Port(
	    ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> BackgroundSpikeSource::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    VertexPortType(ConnectionType::CrossbarInputLabel),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::unique_ptr<grenade::common::Vertex> BackgroundSpikeSource::copy() const
{
	return std::make_unique<BackgroundSpikeSource>(*this);
}

std::unique_ptr<grenade::common::Vertex> BackgroundSpikeSource::move()
{
	return std::make_unique<BackgroundSpikeSource>(std::move(*this));
}

std::ostream& BackgroundSpikeSource::print(std::ostream& os) const
{
	os << "BackgroundSpikeSource(\n" << coordinate << "\n)";
	return os;
}

bool BackgroundSpikeSource::is_equal_to(Vertex const& other) const
{
	auto const& other_bg = static_cast<BackgroundSpikeSource const&>(other);
	return coordinate == other_bg.coordinate && EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
