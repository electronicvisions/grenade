#include "grenade/vx/signal_flow/vertex/synapse_array_view.h"

#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "grenade/vx/signal_flow/vertex/synapse_driver.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synapse_driver.h"
#include <ostream>
#include <set>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

SynapseArrayView::Parameterization::Parameterization(Labels labels, Weights weights) :
    labels(std::move(labels)), weights(std::move(weights))
{
}

std::unique_ptr<grenade::common::PortData> SynapseArrayView::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> SynapseArrayView::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

std::ostream& SynapseArrayView::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n" << hate::Indentation("\t");
	for (size_t i = 0; i < labels.size(); ++i) {
		for (size_t j = 0; j < labels.size(); ++j) {
			ios << i << ", " << j << ": " << labels.at(i).at(j) << ", " << weights.at(i).at(j)
			    << "\n";
		}
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool SynapseArrayView::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	auto const& other_parameterization = static_cast<Parameterization const&>(other);
	return labels == other_parameterization.labels && weights == other_parameterization.weights;
}


SynapseArrayView::SynapseArrayView(
    Synram const& synram,
    Rows rows,
    Columns columns,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    m_synram(synram),
    m_rows(),
    m_columns()
{
	if (rows.empty()) {
		throw std::runtime_error("Row size(0) unsupported.");
	}
	if (columns.empty()) {
		throw std::runtime_error("Column size(0) unsupported.");
	}
	std::set<Columns::value_type> unique_columns(columns.begin(), columns.end());
	if (unique_columns.size() != columns.size()) {
		throw std::runtime_error("Column locations provided to SynapseArrayView are not unique.");
	}
	std::set<Rows::value_type> unique_rows(rows.begin(), rows.end());
	if (unique_rows.size() != rows.size()) {
		throw std::runtime_error("Row locations provided to SynapseArrayView are not unique.");
	}
	m_rows = std::move(rows);
	m_columns = std::move(columns);
}

boost::iterator_range<SynapseArrayView::Rows::const_iterator> SynapseArrayView::get_rows() const
{
	return {m_rows.begin(), m_rows.end()};
}

boost::iterator_range<SynapseArrayView::Columns::const_iterator> SynapseArrayView::get_columns()
    const
{
	return {m_columns.begin(), m_columns.end()};
}

SynapseArrayView::Synram const& SynapseArrayView::get_synram() const
{
	return m_synram;
}

bool SynapseArrayView::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	if (input_port_on_vertex != 1) {
		return false;
	}
	if (auto const parameterization_ptr = dynamic_cast<Parameterization const*>(&data);
	    parameterization_ptr) {
		if (parameterization_ptr->labels.size() != m_rows.size()) {
			return false;
		}
		if (parameterization_ptr->weights.size() != m_rows.size()) {
			return false;
		}
		for (size_t i = 0; i < m_rows.size(); ++i) {
			if (parameterization_ptr->labels.at(i).size() != m_columns.size()) {
				return false;
			}
			if (parameterization_ptr->weights.at(i).size() != m_columns.size()) {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::vector<grenade::common::Vertex::Port> SynapseArrayView::get_input_ports() const
{
	return {
	    grenade::common::Vertex::Port(
	        VertexPortType(ConnectionType::SynapseInputLabel),
	        grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	        grenade::common::CuboidMultiIndexSequence({m_rows.size()})),
	    grenade::common::Vertex::Port(
	        ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> SynapseArrayView::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    VertexPortType(ConnectionType::SynapticInput),
	    grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::CuboidMultiIndexSequence({m_columns.size()}))};
}

bool SynapseArrayView::valid_edge_from(
    Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}
	if (auto const synapse_driver = dynamic_cast<SynapseDriver const*>(&source); synapse_driver) {
		auto const channels_on_target = edge.get_channels_on_target().get_elements();
		auto const channels_on_source = edge.get_channels_on_source().get_elements();
		for (size_t i = 0; i < channels_on_target.size(); ++i) {
			if (synapse_driver->coordinate != halco::hicann_dls::vx::v3::SynapseDriverOnDLS(
			                                      m_rows.at(channels_on_target.at(i).value.at(0))
			                                          .toSynapseDriverOnSynapseDriverBlock(),
			                                      m_synram.toSynapseDriverBlockOnDLS())) {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::unique_ptr<grenade::common::Vertex> SynapseArrayView::copy() const
{
	return std::make_unique<SynapseArrayView>(*this);
}

std::unique_ptr<grenade::common::Vertex> SynapseArrayView::move()
{
	return std::make_unique<SynapseArrayView>(std::move(*this));
}

std::ostream& SynapseArrayView::print(std::ostream& os) const
{
	os << "SynapseArrayView(size: [rows: " << m_rows.size() << ", columns: " << m_columns.size()
	   << "])";
	return os;
}

bool SynapseArrayView::is_equal_to(Vertex const& other) const
{
	auto const& other_synapses = static_cast<SynapseArrayView const&>(other);
	return (m_synram == other_synapses.m_synram) && (m_rows == other_synapses.m_rows) &&
	       (m_columns == other_synapses.m_columns) && EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
