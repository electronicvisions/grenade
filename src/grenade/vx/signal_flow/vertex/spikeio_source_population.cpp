#include "grenade/vx/signal_flow/vertex/spikeio_source_population.h"

#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

SpikeIOSourcePopulation::Parameterization::Parameterization(
    Config const& config, OutputRouting const& output_routing, InputRouting const& input_routing) :
    config(config), output_routing(output_routing), input_routing(input_routing)
{
}

std::unique_ptr<grenade::common::PortData> SpikeIOSourcePopulation::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> SpikeIOSourcePopulation::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

bool SpikeIOSourcePopulation::Parameterization::is_equal_to(PortData const& other) const
{
	return config == static_cast<Parameterization const&>(other).config;
}

std::ostream& SpikeIOSourcePopulation::Parameterization::print(std::ostream& os) const
{
	return os << "Parameterization(" << config << ", "
	          << "output_routing: " << output_routing.size()
	          << " entries, input_routing: " << input_routing.size() << " entries)";
}


SpikeIOSourcePopulation::SpikeIOSourcePopulation(
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor)
{
}

bool SpikeIOSourcePopulation::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	return input_port_on_vertex == 0 && dynamic_cast<Parameterization const*>(&data) != nullptr;
}

bool SpikeIOSourcePopulation::valid_edge_to(
    Vertex const& target, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_to(target, edge)) {
		return false;
	}
	return dynamic_cast<CrossbarL2Input const*>(&target) != nullptr;
}

std::vector<grenade::common::Vertex::Port> SpikeIOSourcePopulation::get_input_ports() const
{
	return {grenade::common::Vertex::Port(
	    ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> SpikeIOSourcePopulation::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    VertexPortType(ConnectionType::CrossbarInputLabel),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::unique_ptr<grenade::common::Vertex> SpikeIOSourcePopulation::copy() const
{
	return std::make_unique<SpikeIOSourcePopulation>(*this);
}

std::unique_ptr<grenade::common::Vertex> SpikeIOSourcePopulation::move()
{
	return std::make_unique<SpikeIOSourcePopulation>(std::move(*this));
}

std::ostream& SpikeIOSourcePopulation::print(std::ostream& os) const
{
	os << "SpikeIOSourcePopulation(";
	EntityOnChip::print(os);
	os << ")";
	return os;
}

bool SpikeIOSourcePopulation::is_equal_to(Vertex const& other) const
{
	return EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
