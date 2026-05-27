#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"

#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"

#include "hate/indent.h"
#include "hate/join.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

CrossbarL2Input::Dynamics::Dynamics(Spikes spikes) : spikes(std::move(spikes)) {}

size_t CrossbarL2Input::Dynamics::batch_size() const
{
	return spikes.size();
}

std::unique_ptr<grenade::common::PortData> CrossbarL2Input::Dynamics::copy() const
{
	return std::make_unique<Dynamics>(*this);
}

std::unique_ptr<grenade::common::PortData> CrossbarL2Input::Dynamics::move()
{
	return std::make_unique<Dynamics>(std::move(*this));
}

bool CrossbarL2Input::Dynamics::is_equal_to(grenade::common::PortData const& other) const
{
	return spikes == static_cast<Dynamics const&>(other).spikes;
}

std::ostream& CrossbarL2Input::Dynamics::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Dynamics(\n";
	for (size_t b = 0; b < batch_size(); b++) {
		ios << hate::Indentation("\t");
		ios << "Batch " << b << "\n";

		ios << hate::Indentation("\t\t");
		ios << hate::join(spikes.at(b), "\n");
	}

	ios << hate::Indentation() << ")";
	return os;
}


CrossbarL2Input::CrossbarL2Input(
    bool requires_input_port_data,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    m_requires_input_port_data(requires_input_port_data)
{
}

bool CrossbarL2Input::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	return input_port_on_vertex == 0 &&
	       (dynamic_cast<Dynamics const*>(&data) != nullptr ||
	        dynamic_cast<CrossbarL2Output::Results const*>(&data) != nullptr ||
	        dynamic_cast<Transformation::Results const*>(&data) != nullptr);
}

bool CrossbarL2Input::valid_edge_to(Vertex const& target, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_to(target, edge)) {
		return false;
	}
	if (auto const crossbar_node = dynamic_cast<CrossbarNode const*>(&target); crossbar_node) {
		return static_cast<bool>(crossbar_node->coordinate.toCrossbarInputOnDLS().toSPL1Address());
	}
	return false;
}

std::vector<grenade::common::Vertex::Port> CrossbarL2Input::get_input_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::TimedSpikeToChipSequence),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> CrossbarL2Input::get_output_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::CrossbarInputLabel),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::unique_ptr<grenade::common::Vertex> CrossbarL2Input::copy() const
{
	return std::make_unique<CrossbarL2Input>(*this);
}

std::unique_ptr<grenade::common::Vertex> CrossbarL2Input::move()
{
	return std::make_unique<CrossbarL2Input>(std::move(*this));
}

std::ostream& CrossbarL2Input::print(std::ostream& os) const
{
	return os << "CrossbarL2Input(requires_input_port_data: " << m_requires_input_port_data << ")";
}

bool CrossbarL2Input::is_equal_to(Vertex const& other) const
{
	return m_requires_input_port_data ==
	           static_cast<CrossbarL2Input const&>(other).m_requires_input_port_data &&
	       EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
