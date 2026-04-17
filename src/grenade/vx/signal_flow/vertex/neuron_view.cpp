#include "grenade/vx/signal_flow/vertex/neuron_view.h"

#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "grenade/vx/signal_flow/vertex/madc_readout.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"

#include <algorithm>
#include <ostream>
#include <set>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

NeuronView::Parameterization::Parameterization(std::vector<Config> configs) :
    configs(std::move(configs))
{
}

bool NeuronView::Parameterization::Config::operator==(
    NeuronView::Parameterization::Config const& other) const
{
	return (atomic_neuron_config == other.atomic_neuron_config) &&
	       (enable_reset == other.enable_reset);
}

bool NeuronView::Parameterization::Config::operator!=(
    NeuronView::Parameterization::Config const& other) const
{
	return !(*this == other);
}


std::unique_ptr<grenade::common::PortData> NeuronView::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> NeuronView::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

std::ostream& NeuronView::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	ios << hate::Indentation("\t");
	for (auto const& config : configs) {
		ios << "Config(\n";
		ios << hate::Indentation("\t\t");
		ios << config.atomic_neuron_config << "\n";
		ios << "enable_reset: " << config.enable_reset << "\n";
		ios << hate::Indentation("\t");
		ios << ")";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool NeuronView::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	return configs == static_cast<Parameterization const&>(other).configs;
}


NeuronView::NeuronView(
    Columns columns,
    Row const& row,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    row(row),
    m_columns()
{
	std::set<Columns::value_type> unique(columns.begin(), columns.end());
	if (unique.size() != columns.size()) {
		throw std::runtime_error("Neuron locations provided to NeuronView are not unique.");
	}
	m_columns = std::move(columns);
}

NeuronView::Columns const& NeuronView::get_columns() const
{
	return m_columns;
}

ppu::NeuronViewHandle NeuronView::toNeuronViewHandle() const
{
	ppu::NeuronViewHandle result;
	for (auto const& column : m_columns) {
		result.columns.push_back(column.value());
	}
	return result;
}

std::vector<grenade::common::Vertex::Port> NeuronView::get_input_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::SynapticInput),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::CuboidMultiIndexSequence({m_columns.size()}))};
}

std::vector<grenade::common::Vertex::Port> NeuronView::get_output_ports() const
{
	return {
	    Port(
	        VertexPortType(ConnectionType::MembraneVoltage),
	        grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	        grenade::common::CuboidMultiIndexSequence({m_columns.size()})),
	    Port(
	        VertexPortType(ConnectionType::CrossbarInputLabel),
	        grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	        grenade::common::CuboidMultiIndexSequence({m_columns.size()}))};
}

std::ostream& NeuronView::print(std::ostream& os) const
{
	os << "NeuronView(row: " << row << ", num_columns: " << m_columns.size() << ")";
	return os;
}

std::unique_ptr<grenade::common::Vertex> NeuronView::copy() const
{
	return std::make_unique<NeuronView>(*this);
}

std::unique_ptr<grenade::common::Vertex> NeuronView::move()
{
	return std::make_unique<NeuronView>(std::move(*this));
}

bool NeuronView::valid_edge_from(Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}
	if (auto const synapse_array_view = dynamic_cast<SynapseArrayView const*>(&source);
	    synapse_array_view) {
		if (synapse_array_view->get_synram() != row.toSynramOnDLS()) {
			return false;
		}
		auto const source_columns = boost::make_iterator_range(synapse_array_view->get_columns());
		auto const source_elements = edge.get_channels_on_source().get_elements();
		auto const target_elements = edge.get_channels_on_target().get_elements();
		for (size_t i = 0; i < source_elements.size(); ++i) {
			if (*(source_columns.begin() + source_elements.at(i).value.at(0)) !=
			    m_columns.at(target_elements.at(i).value.at(0)).toSynapseOnSynapseRow()) {
				return false;
			}
		}
		return true;
	} else if (auto const synapse_array_view_sparse =
	               dynamic_cast<SynapseArrayViewSparse const*>(&source);
	           synapse_array_view_sparse) {
		if (synapse_array_view_sparse->get_synram() != row.toSynramOnDLS()) {
			return false;
		}
		auto const source_columns =
		    boost::make_iterator_range(synapse_array_view_sparse->get_columns());
		auto const source_elements = edge.get_channels_on_source().get_elements();
		auto const target_elements = edge.get_channels_on_target().get_elements();
		for (size_t i = 0; i < source_elements.size(); ++i) {
			if (*(source_columns.begin() + source_elements.at(i).value.at(0)) !=
			    m_columns.at(target_elements.at(i).value.at(0)).toSynapseOnSynapseRow()) {
				return false;
			}
		}
		return true;
	}
	return false;
}

bool NeuronView::valid_edge_to(Vertex const& target, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_to(target, edge)) {
		return false;
	}
	if (auto const crossbar_node = dynamic_cast<CrossbarNode const*>(&target); crossbar_node) {
		return std::any_of(m_columns.begin(), m_columns.end(), [crossbar_node](auto const& column) {
			return column.toNeuronEventOutputOnDLS().toCrossbarInputOnDLS() ==
			       crossbar_node->coordinate.toCrossbarInputOnDLS();
		});
	} else if (auto const madc_recorder = dynamic_cast<MADCReadoutView const*>(&target);
	           madc_recorder) {
		return true;
	} else if (auto const cadc_recorder = dynamic_cast<CADCMembraneReadoutView const*>(&target);
	           cadc_recorder) {
		return true;
	} else if (auto const plasticity_rule = dynamic_cast<PlasticityRule const*>(&target);
	           plasticity_rule) {
		return true;
	}
	return false;
}

bool NeuronView::is_equal_to(Vertex const& other) const
{
	auto const& other_neurons = static_cast<NeuronView const&>(other);
	return (m_columns == other_neurons.m_columns) && (row == other_neurons.row) &&
	       EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
