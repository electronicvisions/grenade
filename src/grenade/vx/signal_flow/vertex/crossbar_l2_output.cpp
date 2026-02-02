#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/routing_crossbar.h"
#include <ostream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

CrossbarL2Output::Results::Results(Spikes spikes) : spikes(std::move(spikes)) {}

size_t CrossbarL2Output::Results::batch_size() const
{
	return spikes.size();
}

std::unique_ptr<grenade::common::PortData> CrossbarL2Output::Results::copy() const
{
	return std::make_unique<Results>(*this);
}

std::unique_ptr<grenade::common::PortData> CrossbarL2Output::Results::move()
{
	return std::make_unique<Results>(std::move(*this));
}

bool CrossbarL2Output::Results::is_equal_to(grenade::common::PortData const& other) const
{
	return spikes == static_cast<Results const&>(other).spikes;
}

std::ostream& CrossbarL2Output::Results::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Results(\n";
	for (size_t b = 0; b < batch_size(); b++) {
		ios << hate::Indentation("\t");
		ios << "Batch " << b << "\n";

		ios << hate::Indentation("\t\t");
		ios << hate::join(spikes.at(b), "\n");
	}

	ios << hate::Indentation() << ")";
	return os;
}


CrossbarL2Output::CrossbarL2Output(
    bool generates_output_port_data,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    m_generates_output_port_data(generates_output_port_data)
{
}

bool CrossbarL2Output::valid_output_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	return input_port_on_vertex == 0 && m_generates_output_port_data &&
	       dynamic_cast<Results const*>(&data) != nullptr;
}

bool CrossbarL2Output::valid_edge_from(
    Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}
	if (auto const crossbar_node = dynamic_cast<CrossbarNode const*>(&source); crossbar_node) {
		return static_cast<bool>(
		    crossbar_node->coordinate.toCrossbarOutputOnDLS().toCrossbarL2OutputOnDLS());
	}
	return false;
}

std::vector<grenade::common::Vertex::Port> CrossbarL2Output::get_input_ports() const
{
	return {grenade::common::Vertex::Port(
	    VertexPortType(ConnectionType::CrossbarOutputLabel),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> CrossbarL2Output::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    VertexPortType(ConnectionType::TimedSpikeFromChipSequence),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	    m_generates_output_port_data ? grenade::common::Vertex::Port::RequiresOrGeneratesData::yes
	                                 : grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::unique_ptr<grenade::common::Vertex> CrossbarL2Output::copy() const
{
	return std::make_unique<CrossbarL2Output>(*this);
}

std::unique_ptr<grenade::common::Vertex> CrossbarL2Output::move()
{
	return std::make_unique<CrossbarL2Output>(std::move(*this));
}

std::ostream& CrossbarL2Output::print(std::ostream& os) const
{
	os << "CrossbarL2Output(generates_output_port_data: " << m_generates_output_port_data << ")";
	return os;
}

bool CrossbarL2Output::is_equal_to(Vertex const& other) const
{
	return m_generates_output_port_data ==
	           static_cast<CrossbarL2Output const&>(other).m_generates_output_port_data &&
	       EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
