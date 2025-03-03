#include "grenade/vx/network/abstract/population_cell/delay.h"

#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/vertex_port_type/spike.h"
#include "hate/indent.h"
#include "hate/join.h"
#include <memory>

namespace grenade::vx::network::abstract {

DelayCell::ParameterSpace::Parameterization::Parameterization(std::vector<common::Time> delays) :
    delays(std::move(delays))
{
}

size_t DelayCell::ParameterSpace::Parameterization::size() const
{
	return delays.size();
}

std::unique_ptr<grenade::common::PortData> DelayCell::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<DelayCell::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> DelayCell::ParameterSpace::Parameterization::move()
{
	return std::make_unique<DelayCell::ParameterSpace::Parameterization>(std::move(*this));
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace::Parameterization>
DelayCell::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	std::vector<common::Time> section_delays;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	section_delays.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		section_delays.push_back(delays.at(element.value.at(0)));
	}
	return std::make_unique<DelayCell::ParameterSpace::Parameterization>(std::move(section_delays));
}

bool DelayCell::ParameterSpace::Parameterization::is_equal_to(
    grenade::common::PortData const& other) const
{
	return delays == static_cast<DelayCell::ParameterSpace::Parameterization const&>(other).delays;
}

std::ostream& DelayCell::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	ios << hate::Indentation("\t");
	ios << hate::join(delays.begin(), delays.end(), "\n");
	ios << hate::Indentation() << "\n)";
	return os;
}


DelayCell::ParameterSpace::ParameterSpace(size_t size) : m_size(size) {}

size_t DelayCell::ParameterSpace::size() const
{
	return m_size;
}

bool DelayCell::ParameterSpace::valid(
    size_t input_port_on_cell,
    grenade::common::Population::Cell::ParameterSpace::Parameterization const& parameterization)
    const
{
	if (input_port_on_cell != 1) {
		return false;
	}
	if (auto const ptr = dynamic_cast<Parameterization const*>(&parameterization); ptr) {
		return size() == ptr->delays.size();
	}
	return false;
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> DelayCell::ParameterSpace::copy()
    const
{
	return std::make_unique<DelayCell::ParameterSpace>(*this);
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> DelayCell::ParameterSpace::move()
{
	return std::make_unique<DelayCell::ParameterSpace>(std::move(*this));
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
DelayCell::ParameterSpace::get_section(grenade::common::MultiIndexSequence const& sequence) const
{
	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	return std::make_unique<DelayCell::ParameterSpace>(sequence.size());
}

bool DelayCell::ParameterSpace::is_equal_to(
    grenade::common::Population::Cell::ParameterSpace const& other) const
{
	return m_size == static_cast<DelayCell::ParameterSpace const&>(other).m_size;
}

std::ostream& DelayCell::ParameterSpace::print(std::ostream& os) const
{
	return os << "ParameterSpace(size: " << m_size << ")";
}


bool DelayCell::requires_time_domain() const
{
	return true;
}

bool DelayCell::is_partitionable() const
{
	return true;
}

bool DelayCell::valid(
    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const
{
	return dynamic_cast<ParameterSpace const*>(&parameter_space) != nullptr;
}

bool DelayCell::valid(size_t, grenade::common::Population::Cell::Dynamics const&) const
{
	return false;
}

bool DelayCell::valid(grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const
{
	return dynamic_cast<ClockCycleTimeDomainRuntimes const*>(&time_domain_runtimes) != nullptr;
}

std::vector<grenade::common::Vertex::Port> DelayCell::get_input_ports() const
{
	return {
	    grenade::common::Vertex::Port(
	        Spike(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	        grenade::common::CuboidMultiIndexSequence(
	            {1}, {grenade::common::CompartmentOnNeuronDimensionUnit()})),
	    grenade::common::Vertex::Port(
	        ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> DelayCell::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    Spike(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::CuboidMultiIndexSequence(
	        {1}, {grenade::common::CompartmentOnNeuronDimensionUnit()}))};
}

std::unique_ptr<grenade::common::Population::Cell> DelayCell::copy() const
{
	return std::make_unique<DelayCell>(*this);
}

std::unique_ptr<grenade::common::Population::Cell> DelayCell::move()
{
	return std::make_unique<DelayCell>(std::move(*this));
}

bool DelayCell::is_equal_to(grenade::common::Population::Cell const&) const
{
	return true;
}

std::ostream& DelayCell::print(std::ostream& os) const
{
	return os << "DelayCell()";
}

} // namespace grenade::vx::network::abstract
