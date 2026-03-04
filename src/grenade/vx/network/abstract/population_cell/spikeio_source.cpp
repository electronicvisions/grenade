#include "grenade/vx/network/abstract/population_cell/spikeio_source.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/population.h"
#include "grenade/common/port_data.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/vertex_port_type/spike.h"
#include "hate/indent.h"

#include <stdexcept>
#include <utility>

namespace grenade::vx::network::abstract {

size_t SpikeIOSourceNeuron::ParameterSpace::Parameterization::size() const
{
	return labels.size();
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace::Parameterization>
SpikeIOSourceNeuron::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}

	auto ret = std::make_unique<SpikeIOSourceNeuron::ParameterSpace::Parameterization>();
	ret->data_rate_scaler = data_rate_scaler;

	ret->labels.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret->labels.push_back(labels.at(element.value.at(0)));
	}

	return ret;
}

std::unique_ptr<grenade::common::PortData>
SpikeIOSourceNeuron::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<SpikeIOSourceNeuron::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData>
SpikeIOSourceNeuron::ParameterSpace::Parameterization::move()
{
	return std::make_unique<SpikeIOSourceNeuron::ParameterSpace::Parameterization>(
	    std::move(*this));
}

bool SpikeIOSourceNeuron::ParameterSpace::Parameterization::is_equal_to(
    grenade::common::PortData const& other) const
{
	auto const& o =
	    static_cast<SpikeIOSourceNeuron::ParameterSpace::Parameterization const&>(other);
	return labels == o.labels && data_rate_scaler == o.data_rate_scaler;
}

std::ostream& SpikeIOSourceNeuron::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	ios << hate::Indentation("\t");
	ios << "labels:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& label : labels) {
		ios << label << "\n";
	}
	ios << hate::Indentation("\t");
	ios << "data_rate_scaler: " << data_rate_scaler << "\n";
	ios << hate::Indentation();
	ios << ")";
	return os;
}


SpikeIOSourceNeuron::ParameterSpace::ParameterSpace(size_t size) : m_size(size) {}

size_t SpikeIOSourceNeuron::ParameterSpace::size() const
{
	return m_size;
}

bool SpikeIOSourceNeuron::ParameterSpace::valid(
    size_t input_port_on_cell,
    grenade::common::Population::Cell::ParameterSpace::Parameterization const& parameterization)
    const
{
	if (input_port_on_cell != 0) {
		return false;
	}
	if (auto const ptr = dynamic_cast<Parameterization const*>(&parameterization); ptr) {
		if (m_size != ptr->size()) {
			return false;
		}
		return true;
	}
	return false;
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
SpikeIOSourceNeuron::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	return std::make_unique<SpikeIOSourceNeuron::ParameterSpace>(sequence.size());
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
SpikeIOSourceNeuron::ParameterSpace::copy() const
{
	return std::make_unique<SpikeIOSourceNeuron::ParameterSpace>(*this);
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
SpikeIOSourceNeuron::ParameterSpace::move()
{
	return std::make_unique<SpikeIOSourceNeuron::ParameterSpace>(std::move(*this));
}

bool SpikeIOSourceNeuron::ParameterSpace::is_equal_to(
    grenade::common::Population::Cell::ParameterSpace const& other) const
{
	return m_size == static_cast<SpikeIOSourceNeuron::ParameterSpace const&>(other).m_size;
}

std::ostream& SpikeIOSourceNeuron::ParameterSpace::print(std::ostream& os) const
{
	os << "ParameterSpace(" << m_size << ")";
	return os;
}


bool SpikeIOSourceNeuron::valid(
    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const
{
	return dynamic_cast<ParameterSpace const*>(&parameter_space) != nullptr;
}

bool SpikeIOSourceNeuron::valid(
    size_t /* input_port_on_cell */,
    grenade::common::Population::Cell::Dynamics const& /* dynamics */) const
{
	return false;
}

bool SpikeIOSourceNeuron::requires_time_domain() const
{
	return true;
}

bool SpikeIOSourceNeuron::is_partitionable() const
{
	return false;
}

bool SpikeIOSourceNeuron::valid(
    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const
{
	return dynamic_cast<ClockCycleTimeDomainRuntimes const*>(&time_domain_runtimes) != nullptr;
}

std::vector<grenade::common::Vertex::Port> SpikeIOSourceNeuron::get_input_ports() const
{
	return {grenade::common::Vertex::Port(
	    ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> SpikeIOSourceNeuron::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    Spike(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::CuboidMultiIndexSequence(
	        {1}, {grenade::common::CompartmentOnNeuronDimensionUnit()}))};
}

std::unique_ptr<grenade::common::Population::Cell> SpikeIOSourceNeuron::copy() const
{
	return std::make_unique<SpikeIOSourceNeuron>(*this);
}

std::unique_ptr<grenade::common::Population::Cell> SpikeIOSourceNeuron::move()
{
	return std::make_unique<SpikeIOSourceNeuron>(std::move(*this));
}

bool SpikeIOSourceNeuron::is_equal_to(grenade::common::Population::Cell const&) const
{
	return true;
}

std::ostream& SpikeIOSourceNeuron::print(std::ostream& os) const
{
	return os << "SpikeIOSourceNeuron()";
}

} // namespace grenade::vx::network::abstract