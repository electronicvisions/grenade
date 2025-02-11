#include "grenade/vx/network/abstract/population_cell/poisson_source.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/vertex_port_type/spike.h"
#include "hate/indent.h"
#include "hate/join.h"

namespace grenade::vx::network::abstract {

PoissonSourceNeuron::ParameterSpace::Parameterization::Parameterization(size_t size) : m_size(size)
{
}

size_t PoissonSourceNeuron::ParameterSpace::Parameterization::size() const
{
	return m_size;
}

std::unique_ptr<grenade::common::PortData>
PoissonSourceNeuron::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<PoissonSourceNeuron::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData>
PoissonSourceNeuron::ParameterSpace::Parameterization::move()
{
	return std::make_unique<PoissonSourceNeuron::ParameterSpace::Parameterization>(
	    std::move(*this));
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace::Parameterization>
PoissonSourceNeuron::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	auto ret = std::make_unique<Parameterization>(*this);
	assert(ret);
	ret->m_size = sequence.size();
	return ret;
}

bool PoissonSourceNeuron::ParameterSpace::Parameterization::is_equal_to(
    grenade::common::PortData const& other) const
{
	auto const& other_poisson =
	    static_cast<PoissonSourceNeuron::ParameterSpace::Parameterization const&>(other);
	return m_size == other_poisson.m_size && period == other_poisson.period &&
	       rate == other_poisson.rate && seed == other_poisson.seed;
}

std::ostream& PoissonSourceNeuron::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	ios << hate::Indentation("\t");
	ios << "size: " << m_size << "\n";
	ios << "period: " << period << "\n";
	ios << "rate: " << rate << "\n";
	ios << "seed: " << seed;
	ios << hate::Indentation();
	ios << "\n)";
	return os;
}


PoissonSourceNeuron::ParameterSpace::ParameterSpace(size_t size) : m_size(size) {}

size_t PoissonSourceNeuron::ParameterSpace::size() const
{
	return m_size;
}

bool PoissonSourceNeuron::ParameterSpace::valid(
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
PoissonSourceNeuron::ParameterSpace::copy() const
{
	return std::make_unique<PoissonSourceNeuron::ParameterSpace>(*this);
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
PoissonSourceNeuron::ParameterSpace::move()
{
	return std::make_unique<PoissonSourceNeuron::ParameterSpace>(std::move(*this));
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
PoissonSourceNeuron::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	return std::make_unique<PoissonSourceNeuron::ParameterSpace>(sequence.size());
}

bool PoissonSourceNeuron::ParameterSpace::is_equal_to(
    grenade::common::Population::Cell::ParameterSpace const& other) const
{
	return m_size == static_cast<PoissonSourceNeuron::ParameterSpace const&>(other).m_size;
}

std::ostream& PoissonSourceNeuron::ParameterSpace::print(std::ostream& os) const
{
	os << "ParameterSpace(" << m_size << ")";
	return os;
}


bool PoissonSourceNeuron::requires_time_domain() const
{
	return true;
}

bool PoissonSourceNeuron::is_partitionable() const
{
	return true;
}

bool PoissonSourceNeuron::valid(
    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const
{
	return dynamic_cast<ParameterSpace const*>(&parameter_space) != nullptr;
}

bool PoissonSourceNeuron::valid(
    size_t, grenade::common::Population::Cell::Dynamics const& /* dynamics */) const
{
	return false;
}

bool PoissonSourceNeuron::valid(
    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const
{
	return dynamic_cast<ClockCycleTimeDomainRuntimes const*>(&time_domain_runtimes) != nullptr;
}


std::vector<grenade::common::Vertex::Port> PoissonSourceNeuron::get_input_ports() const
{
	return {grenade::common::Vertex::Port(
	    ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> PoissonSourceNeuron::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    Spike(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::CuboidMultiIndexSequence(
	        {1}, {grenade::common::CompartmentOnNeuronDimensionUnit()}))};
}

std::unique_ptr<grenade::common::Population::Cell> PoissonSourceNeuron::copy() const
{
	return std::make_unique<PoissonSourceNeuron>(*this);
}

std::unique_ptr<grenade::common::Population::Cell> PoissonSourceNeuron::move()
{
	return std::make_unique<PoissonSourceNeuron>(std::move(*this));
}

bool PoissonSourceNeuron::is_equal_to(grenade::common::Population::Cell const&) const
{
	return true;
}

std::ostream& PoissonSourceNeuron::print(std::ostream& os) const
{
	return os << "PoissonSourceNeuron()";
}

} // namespace grenade::vx::network::abstract
