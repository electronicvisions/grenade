#include "grenade/vx/signal_flow/vertex/crossbar_node.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/signal_flow/vertex/background_spike_source.h"
#include <memory>
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

CrossbarNode::Parameterization::Parameterization(haldls::vx::v3::CrossbarNode config) :
    config(std::move(config))
{
}

std::unique_ptr<grenade::common::PortData> CrossbarNode::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> CrossbarNode::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

bool CrossbarNode::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	return config == static_cast<Parameterization const&>(other).config;
}

std::ostream& CrossbarNode::Parameterization::print(std::ostream& os) const
{
	return os << "Parameterization(" << config << ")";
}


CrossbarNode::CrossbarNode(
    Coordinate const& coordinate,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    coordinate(coordinate)
{}

bool CrossbarNode::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& port_data) const
{
	return input_port_on_vertex == 1 &&
	       dynamic_cast<Parameterization const*>(&port_data) != nullptr;
}

std::vector<grenade::common::Vertex::Port> CrossbarNode::get_input_ports() const
{
	return {
	    grenade::common::Vertex::Port(
	        VertexPortType(ConnectionType::CrossbarInputLabel),
	        coordinate.toCrossbarInputOnDLS().toNeuronEventOutputOnDLS()
	            ? grenade::common::Vertex::Port::SumOrSplitSupport::yes
	            : grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})})),
	    grenade::common::Vertex::Port(
	        ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> CrossbarNode::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    VertexPortType(ConnectionType::CrossbarOutputLabel),
	    grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::unique_ptr<grenade::common::Vertex> CrossbarNode::copy() const
{
	return std::make_unique<CrossbarNode>(*this);
}

std::unique_ptr<grenade::common::Vertex> CrossbarNode::move()
{
	return std::make_unique<CrossbarNode>(std::move(*this));
}

std::ostream& CrossbarNode::print(std::ostream& os) const
{
	os << "CrossbarNode(" << coordinate << ")";
	return os;
}

bool CrossbarNode::is_equal_to(Vertex const& other) const
{
	auto const& other_node = static_cast<CrossbarNode const&>(other);
	return (coordinate == other_node.coordinate) && EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
