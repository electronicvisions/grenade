#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"

#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"

#include <numeric>
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

CADCMembraneReadoutView::Results::Results(Samples samples) : samples(std::move(samples)) {}

size_t CADCMembraneReadoutView::Results::batch_size() const
{
	return samples.size();
}

std::unique_ptr<grenade::common::PortData> CADCMembraneReadoutView::Results::copy() const
{
	return std::make_unique<Results>(*this);
}

std::unique_ptr<grenade::common::PortData> CADCMembraneReadoutView::Results::move()
{
	return std::make_unique<Results>(std::move(*this));
}

std::ostream& CADCMembraneReadoutView::Results::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Results(\n";
	for (size_t b = 0; auto const& batch_entry : samples) {
		ios << hate::Indentation("\t");
		ios << b << ":\n";
		ios << hate::Indentation("\t\t");
		for (auto const& samples : batch_entry) {
			ios << samples.time << ": " << hate::join(samples.data, ", ") << "\n";
		}
		b++;
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool CADCMembraneReadoutView::Results::is_equal_to(grenade::common::PortData const& other) const
{
	return samples == static_cast<Results const&>(other).samples;
}


CADCMembraneReadoutView::CADCMembraneReadoutView(
    Columns columns,
    Synram const& synram,
    Mode const& mode,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    m_columns(),
    m_synram(synram),
    m_mode(mode)
{
	std::set<Columns::value_type::value_type> unique{columns.begin(), columns.end()};
	if (unique.size() != columns.size()) {
		throw std::runtime_error("Column locations provided to CADCReadoutView are not unique.");
	}
	m_columns = std::move(columns);
}

bool CADCMembraneReadoutView::valid_output_port_data(
    size_t output_port_on_vertex, grenade::common::PortData const& data) const
{
	return output_port_on_vertex == 0 && dynamic_cast<Results const*>(&data) != nullptr;
}

CADCMembraneReadoutView::Columns const& CADCMembraneReadoutView::get_columns() const
{
	return m_columns;
}

CADCMembraneReadoutView::Synram const& CADCMembraneReadoutView::get_synram() const
{
	return m_synram;
}

CADCMembraneReadoutView::Mode const& CADCMembraneReadoutView::get_mode() const
{
	return m_mode;
}

std::vector<grenade::common::Vertex::Port> CADCMembraneReadoutView::get_input_ports() const
{
	return {grenade::common::Vertex::Port(
	    VertexPortType(ConnectionType::MembraneVoltage),
	    grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::CuboidMultiIndexSequence({m_columns.size()}))};
}

std::vector<grenade::common::Vertex::Port> CADCMembraneReadoutView::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    VertexPortType(ConnectionType::Int8), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::CuboidMultiIndexSequence({m_columns.size()}))};
}

std::ostream& CADCMembraneReadoutView::print(std::ostream& os) const
{
	return os << "CADCMembraneReadoutView(size: " << m_columns.size() << ")";
}

bool CADCMembraneReadoutView::valid_edge_from(
    Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}
	if (auto const neuron_ptr = dynamic_cast<NeuronView const*>(&source); neuron_ptr) {
		auto const channels_on_target = edge.get_channels_on_target().get_elements();
		auto const channels_on_source = edge.get_channels_on_source().get_elements();
		if (neuron_ptr->row != m_synram.toNeuronRowOnDLS()) {
			return false;
		}
		for (size_t i = 0; i < channels_on_target.size(); ++i) {
			if (neuron_ptr->get_columns().at(channels_on_source.at(i).value.at(0)) !=
			    m_columns.at(channels_on_target.at(i).value.at(0))) {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::unique_ptr<grenade::common::Vertex> CADCMembraneReadoutView::copy() const
{
	return std::make_unique<CADCMembraneReadoutView>(*this);
}

std::unique_ptr<grenade::common::Vertex> CADCMembraneReadoutView::move()
{
	return std::make_unique<CADCMembraneReadoutView>(std::move(*this));
}

bool CADCMembraneReadoutView::is_equal_to(Vertex const& other) const
{
	auto const& other_cadc = static_cast<CADCMembraneReadoutView const&>(other);
	return (m_columns == other_cadc.m_columns) && (m_synram == other_cadc.m_synram) &&
	       (m_mode == other_cadc.m_mode) && EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
