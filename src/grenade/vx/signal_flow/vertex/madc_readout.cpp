#include "grenade/vx/signal_flow/vertex/madc_readout.h"

#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "hate/indent.h"
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

MADCReadoutView::Results::Results(Samples samples) : samples(std::move(samples)) {}

size_t MADCReadoutView::Results::batch_size() const
{
	return samples.size();
}

std::unique_ptr<grenade::common::PortData> MADCReadoutView::Results::copy() const
{
	return std::make_unique<Results>(*this);
}

std::unique_ptr<grenade::common::PortData> MADCReadoutView::Results::move()
{
	return std::make_unique<Results>(std::move(*this));
}

std::ostream& MADCReadoutView::Results::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Results(\n";
	for (size_t b = 0; auto const& batch_entry : samples) {
		ios << hate::Indentation("\t");
		ios << b << ":\n";
		ios << hate::Indentation("\t\t");
		for (auto const& sample : batch_entry) {
			ios << sample.time << ": " << sample.data << "\n";
		}
		b++;
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool MADCReadoutView::Results::is_equal_to(grenade::common::PortData const& other) const
{
	return samples == static_cast<Results const&>(other).samples;
}


MADCReadoutView::Parameterization::Parameterization() {}

std::unique_ptr<grenade::common::PortData> MADCReadoutView::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> MADCReadoutView::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

std::ostream& MADCReadoutView::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	ios << hate::Indentation("\t");
	ios << initial << "\n";
	ios << period << "\n";
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool MADCReadoutView::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	auto const& other_parameterization = static_cast<Parameterization const&>(other);
	return initial == other_parameterization.initial && period == other_parameterization.period;
}


MADCReadoutView::MADCReadoutView(
    Source const& first_source,
    std::optional<Source> const& second_source,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    m_first_source(first_source),
    m_second_source()
{
	if (second_source) {
		if ((first_source.toNeuronColumnOnDLS().value() % 2) ==
		    second_source->toNeuronColumnOnDLS().value() % 2) {
			throw std::invalid_argument(
			    "MADCReadoutView sources can't both be located in even or odd neuron columns.");
		}
	}
	m_second_source = second_source;
}

MADCReadoutView::Source const& MADCReadoutView::get_first_source() const
{
	return m_first_source;
}

std::optional<MADCReadoutView::Source> const& MADCReadoutView::get_second_source() const
{
	return m_second_source;
}

bool MADCReadoutView::valid_output_port_data(
    size_t output_port_on_vertex, grenade::common::PortData const& data) const
{
	return output_port_on_vertex == 0 && dynamic_cast<Results const*>(&data) != nullptr;
}

bool MADCReadoutView::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	if (input_port_on_vertex != 1) {
		return false;
	}
	if (auto const parameterization_ptr = dynamic_cast<Parameterization const*>(&data);
	    parameterization_ptr != nullptr) {
		if (!m_second_source && ((parameterization_ptr->period.value() != 0) ||
		                         (parameterization_ptr->initial.value() != 0))) {
			// MADCReadoutView source swapping only possible for two sources.
			return false;
		}
		if (parameterization_ptr->period > Parameterization::Period(1)) {
			// MADCReadoutView source switch period > 1 not supported due to Issue #3998.
			return false;
		}
		return true;
	}
	return false;
}

std::vector<grenade::common::Vertex::Port> MADCReadoutView::get_input_ports() const
{
	std::vector<grenade::common::MultiIndex> channels;
	channels.push_back(grenade::common::MultiIndex({0}));
	if (m_second_source) {
		channels.push_back(grenade::common::MultiIndex({1}));
	}
	return {
	    grenade::common::Vertex::Port(
	        VertexPortType(ConnectionType::MembraneVoltage),
	        grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	        grenade::common::ListMultiIndexSequence(channels)),
	    grenade::common::Vertex::Port(
	        ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> MADCReadoutView::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    VertexPortType(ConnectionType::TimedMADCSampleFromChipSequence),
	    grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::ostream& MADCReadoutView::print(std::ostream& os) const
{
	os << "MADCReadoutView(";
	if (m_second_source) {
		os << "MADCReadoutView(first_coord: " << m_first_source
		   << ", second_coord: " << *m_second_source << ")";
	} else {
		os << "MADCReadoutView(coord: " << m_first_source << ")";
	}
	return os;
}

bool MADCReadoutView::valid_edge_from(Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}
	if (auto const neuron_ptr = dynamic_cast<NeuronView const*>(&source); neuron_ptr) {
		auto const channels_on_target = edge.get_channels_on_target().get_elements();
		auto const channels_on_source = edge.get_channels_on_source().get_elements();
		for (size_t i = 0; i < channels_on_target.size(); ++i) {
			if (halco::hicann_dls::vx::v3::AtomicNeuronOnDLS(
			        neuron_ptr->get_columns().at(channels_on_source.at(i).value.at(0)),
			        neuron_ptr->row) !=
			    (channels_on_target.at(i).value.at(0) == 1 ? *m_second_source : m_first_source)) {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::unique_ptr<grenade::common::Vertex> MADCReadoutView::copy() const
{
	return std::make_unique<MADCReadoutView>(*this);
}

std::unique_ptr<grenade::common::Vertex> MADCReadoutView::move()
{
	return std::make_unique<MADCReadoutView>(std::move(*this));
}

bool MADCReadoutView::is_equal_to(Vertex const& other) const
{
	auto const& other_madc = static_cast<MADCReadoutView const&>(other);
	return (m_first_source == other_madc.m_first_source) &&
	       (m_second_source == other_madc.m_second_source) && EntityOnChip::is_equal_to(other);
}

} // namespace grenade::vx::signal_flow::vertex
