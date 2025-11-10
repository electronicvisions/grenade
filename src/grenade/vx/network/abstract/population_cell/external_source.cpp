#include "grenade/vx/network/abstract/population_cell/external_source.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/vertex_port_type/spike.h"
#include "hate/indent.h"
#include "hate/join.h"

namespace grenade::vx::network::abstract {

ExternalSourceNeuron::ParameterSpace::ParameterSpace(size_t size) : m_size(size) {}

size_t ExternalSourceNeuron::ParameterSpace::size() const
{
	return m_size;
}

bool ExternalSourceNeuron::ParameterSpace::valid(
    size_t,
    grenade::common::Population::Cell::ParameterSpace::
        Parameterization const& /* parameterization */) const
{
	return false;
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
ExternalSourceNeuron::ParameterSpace::copy() const
{
	return std::make_unique<ExternalSourceNeuron::ParameterSpace>(*this);
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
ExternalSourceNeuron::ParameterSpace::move()
{
	return std::make_unique<ExternalSourceNeuron::ParameterSpace>(std::move(*this));
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
ExternalSourceNeuron::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	return std::make_unique<ExternalSourceNeuron::ParameterSpace>(sequence.size());
}

bool ExternalSourceNeuron::ParameterSpace::is_equal_to(
    grenade::common::Population::Cell::ParameterSpace const& other) const
{
	return m_size == static_cast<ExternalSourceNeuron::ParameterSpace const&>(other).m_size;
}

std::ostream& ExternalSourceNeuron::ParameterSpace::print(std::ostream& os) const
{
	os << "ParameterSpace(" << m_size << ")";
	return os;
}


ExternalSourceNeuron::Dynamics::Dynamics(SpikeTimes spike_times) :
    spike_times(std::move(spike_times))
{
}

size_t ExternalSourceNeuron::Dynamics::size() const
{
	if (spike_times.empty()) {
		return 0;
	}
	size_t s = spike_times.at(0).size();
	for (auto const& batch_entry : spike_times) {
		if (batch_entry.size() != s) {
			throw std::runtime_error("SpikeTimes size not homogeneous across batch entries.");
		}
	}
	return s;
}

size_t ExternalSourceNeuron::Dynamics::batch_size() const
{
	return spike_times.size();
}

std::unique_ptr<grenade::common::Population::Cell::Dynamics>
ExternalSourceNeuron::Dynamics::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	SpikeTimes section_spike_times(batch_size());
	for (size_t b = 0; b < batch_size(); ++b) {
		for (auto const& element : sequence.get_elements()) {
			section_spike_times.at(b).push_back(spike_times.at(b).at(element.value.at(0)));
		}
	}
	return std::make_unique<ExternalSourceNeuron::Dynamics>(std::move(section_spike_times));
}

std::unique_ptr<grenade::common::PortData> ExternalSourceNeuron::Dynamics::copy() const
{
	return std::make_unique<ExternalSourceNeuron::Dynamics>(*this);
}

std::unique_ptr<grenade::common::PortData> ExternalSourceNeuron::Dynamics::move()
{
	return std::make_unique<ExternalSourceNeuron::Dynamics>(std::move(*this));
}

bool ExternalSourceNeuron::Dynamics::is_equal_to(grenade::common::PortData const& other) const
{
	return spike_times == static_cast<ExternalSourceNeuron::Dynamics const&>(other).spike_times;
}

std::ostream& ExternalSourceNeuron::Dynamics::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Dynamics(\n";
	for (size_t b = 0; auto const& batch_entry : spike_times) {
		ios << hate::Indentation("\t");
		ios << "batch_entry " << b << ":\n";
		ios << hate::Indentation("\t\t");
		for (size_t n = 0; auto const& neuron : batch_entry) {
			ios << "neuron " << n << ":\n";
			ios << hate::Indentation("\t\t\t");
			for (auto const& spike_time : neuron) {
				ios << spike_time << "\n";
			}
			n++;
		}
		b++;
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}


bool ExternalSourceNeuron::requires_time_domain() const
{
	return true;
}

bool ExternalSourceNeuron::is_partitionable() const
{
	return true;
}

bool ExternalSourceNeuron::valid(
    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const
{
	return dynamic_cast<ParameterSpace const*>(&parameter_space) != nullptr;
}

bool ExternalSourceNeuron::valid(
    size_t input_port_on_cell, grenade::common::Population::Cell::Dynamics const& dynamics) const
{
	return input_port_on_cell == 0 && dynamic_cast<Dynamics const*>(&dynamics) != nullptr;
}

bool ExternalSourceNeuron::valid(
    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const
{
	return dynamic_cast<ClockCycleTimeDomainRuntimes const*>(&time_domain_runtimes) != nullptr;
}


std::vector<grenade::common::Vertex::Port> ExternalSourceNeuron::get_input_ports() const
{
	return {grenade::common::Vertex::Port(
	    DynamicsPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> ExternalSourceNeuron::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    Spike(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::CuboidMultiIndexSequence(
	        {1}, {grenade::common::CompartmentOnNeuronDimensionUnit()}))};
}

std::unique_ptr<grenade::common::Population::Cell> ExternalSourceNeuron::copy() const
{
	return std::make_unique<ExternalSourceNeuron>(*this);
}

std::unique_ptr<grenade::common::Population::Cell> ExternalSourceNeuron::move()
{
	return std::make_unique<ExternalSourceNeuron>(std::move(*this));
}

bool ExternalSourceNeuron::is_equal_to(grenade::common::Population::Cell const&) const
{
	return true;
}

std::ostream& ExternalSourceNeuron::print(std::ostream& os) const
{
	return os << "ExternalSourceNeuron()";
}

} // namespace grenade::vx::network::abstract
