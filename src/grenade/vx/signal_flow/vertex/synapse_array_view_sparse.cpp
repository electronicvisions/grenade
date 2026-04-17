#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"

#include "grenade/common/edge.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "grenade/vx/signal_flow/vertex/synapse_driver.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synapse_driver.h"
#include "hate/indent.h"
#include <ostream>
#include <set>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

SynapseArrayViewSparse::Parameterization::Parameterization(Labels labels, Weights weights) :
    labels(std::move(labels)), weights(std::move(weights))
{
}

std::unique_ptr<grenade::common::PortData> SynapseArrayViewSparse::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> SynapseArrayViewSparse::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

std::ostream& SynapseArrayViewSparse::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n" << hate::Indentation("\t");
	for (size_t i = 0; i < labels.size(); ++i) {
		ios << labels.at(i) << ", " << weights.at(i) << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool SynapseArrayViewSparse::Parameterization::is_equal_to(PortData const& other) const
{
	auto const& other_parameterization = static_cast<Parameterization const&>(other);
	return labels == other_parameterization.labels && weights == other_parameterization.weights;
}


bool SynapseArrayViewSparse::Synapse::operator==(Synapse const& other) const
{
	return (index_row == other.index_row) && (index_column == other.index_column);
}

bool SynapseArrayViewSparse::Synapse::operator!=(Synapse const& other) const
{
	return !(*this == other);
}


void SynapseArrayViewSparse::check(
    Rows const& rows, Columns const& columns, Synapses const& synapses)
{
	if (rows.empty()) {
		throw std::runtime_error("Row size(0) unsupported.");
	}
	if (columns.empty()) {
		throw std::runtime_error("Column size(0) unsupported.");
	}
	std::set<Columns::value_type> unique_columns(columns.begin(), columns.end());
	if (unique_columns.size() != columns.size()) {
		throw std::runtime_error(
		    "Column locations provided to SynapseArrayViewSparse are not unique.");
	}
	std::set<Rows::value_type> unique_rows(rows.begin(), rows.end());
	if (unique_rows.size() != rows.size()) {
		throw std::runtime_error(
		    "Row locations provided to SynapseArrayViewSparse are not unique.");
	}
	std::set<std::pair<size_t, size_t>> unique_synapses;
	for (auto const& synapse : synapses) {
		if (synapse.index_row >= rows.size()) {
			throw std::runtime_error("Synapse row index out of range.");
		}
		if (synapse.index_column >= columns.size()) {
			throw std::runtime_error("Synapse column index out of range.");
		}
		unique_synapses.insert({synapse.index_row, synapse.index_column});
	}
	if (unique_synapses.size() != synapses.size()) {
		throw std::runtime_error(
		    "Synapse locations provided to SynapseArrayViewSparse are not unique.");
	}
}

SynapseArrayViewSparse::SynapseArrayViewSparse(
    Synram synram,
    Rows rows_in,
    Columns columns_in,
    Synapses synapses_in,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    synram(synram),
    rows(),
    columns(),
    synapses()
{
	check(rows_in, columns_in, synapses_in);
	rows = std::move(rows_in);
	columns = std::move(columns_in);
	synapses = std::move(synapses_in);
}

boost::iterator_range<SynapseArrayViewSparse::Rows::const_iterator>
SynapseArrayViewSparse::get_rows() const
{
	return {rows.begin(), rows.end()};
}

boost::iterator_range<SynapseArrayViewSparse::Columns::const_iterator>
SynapseArrayViewSparse::get_columns() const
{
	return {columns.begin(), columns.end()};
}

boost::iterator_range<SynapseArrayViewSparse::Synapses::const_iterator>
SynapseArrayViewSparse::get_synapses() const
{
	return {synapses.begin(), synapses.end()};
}

SynapseArrayViewSparse::Synram const& SynapseArrayViewSparse::get_synram() const
{
	return synram;
}

ppu::SynapseArrayViewHandle SynapseArrayViewSparse::toSynapseArrayViewHandle() const
{
	std::set<halco::hicann_dls::vx::v3::SynapseRowOnSynram> used_rows;
	std::set<halco::hicann_dls::vx::v3::SynapseOnSynapseRow> used_columns;
	for (auto const& synapse : synapses) {
		used_rows.insert(rows.at(synapse.index_row));
		used_columns.insert(columns.at(synapse.index_column));
	}

	if (synapses.size() != used_rows.size() * used_columns.size()) {
		throw std::runtime_error(
		    "Conversion to SynapseArrayViewHandle only supported for dense connectivity.");
	}
	ppu::SynapseArrayViewHandle result;
	for (auto const& column : used_columns) {
		result.columns.push_back(column.value());
	}
	for (auto const& row : used_rows) {
		result.rows.push_back(row.value());
	}
	return result;
}

bool SynapseArrayViewSparse::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	if (input_port_on_vertex != 1) {
		return false;
	}
	if (auto const parameterization_ptr = dynamic_cast<Parameterization const*>(&data);
	    parameterization_ptr) {
		if (parameterization_ptr->labels.size() != synapses.size()) {
			return false;
		}
		if (parameterization_ptr->weights.size() != synapses.size()) {
			return false;
		}
		return true;
	}
	return false;
}

std::vector<grenade::common::Vertex::Port> SynapseArrayViewSparse::get_input_ports() const
{
	return {
	    grenade::common::Vertex::Port(
	        VertexPortType(ConnectionType::SynapseInputLabel),
	        grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	        grenade::common::CuboidMultiIndexSequence({rows.size()})),
	    grenade::common::Vertex::Port(
	        ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> SynapseArrayViewSparse::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    VertexPortType(ConnectionType::SynapticInput),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes, // only yes because of plasticity
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::CuboidMultiIndexSequence({columns.size()}))};
}

bool SynapseArrayViewSparse::valid_edge_from(
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
			                                      rows.at(channels_on_target.at(i).value.at(0))
			                                          .toSynapseDriverOnSynapseDriverBlock(),
			                                      synram.toSynapseDriverBlockOnDLS())) {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::unique_ptr<grenade::common::Vertex> SynapseArrayViewSparse::copy() const
{
	return std::make_unique<SynapseArrayViewSparse>(*this);
}

std::unique_ptr<grenade::common::Vertex> SynapseArrayViewSparse::move()
{
	return std::make_unique<SynapseArrayViewSparse>(std::move(*this));
}

std::ostream& SynapseArrayViewSparse::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "SynapseArrayViewSparse(\n" << hate::Indentation("\t");
	ios << "size: [rows: " << rows.size() << ", columns: " << columns.size() << "]\n";
	for (auto const& synapse : synapses) {
		ios << "(" << rows.at(synapse.index_row) << ", " << columns.at(synapse.index_column)
		    << ")\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool SynapseArrayViewSparse::is_equal_to(Vertex const& other) const
{
	auto const& other_synapses = static_cast<SynapseArrayViewSparse const&>(other);
	return (synram == other_synapses.synram) && (rows == other_synapses.rows) &&
	       (columns == other_synapses.columns) && (synapses == other_synapses.synapses) &&
	       EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
