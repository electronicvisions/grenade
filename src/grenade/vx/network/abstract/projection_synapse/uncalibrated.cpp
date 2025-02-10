#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"

#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/projection.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/vertex_port_type/spike.h"
#include "grenade/vx/network/abstract/vertex_port_type/synapse_observable.h"
#include "grenade/vx/network/abstract/vertex_port_type/synaptic_input.h"
#include "grenade/vx/network/abstract/vertex_port_type/weight.h"
#include "hate/indent.h"
#include "hate/join.h"
#include <memory>

namespace grenade::vx::network::abstract {

UncalibratedSynapse::ParameterSpace::Parameterization::Parameterization(
    std::vector<Weight> weights) :
    weights(std::move(weights))
{
}

size_t UncalibratedSynapse::ParameterSpace::Parameterization::size() const
{
	return weights.size();
}

std::unique_ptr<grenade::common::Projection::Synapse::ParameterSpace::Parameterization>
UncalibratedSynapse::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	std::vector<Weight> section_weights;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	for (auto const& element : sequence.get_elements()) {
		section_weights.push_back(weights.at(element.value.at(0)));
	}
	return std::make_unique<Parameterization>(std::move(section_weights));
}

std::unique_ptr<grenade::common::PortData>
UncalibratedSynapse::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<UncalibratedSynapse::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData>
UncalibratedSynapse::ParameterSpace::Parameterization::move()
{
	return std::make_unique<UncalibratedSynapse::ParameterSpace::Parameterization>(
	    std::move(*this));
}

bool UncalibratedSynapse::ParameterSpace::Parameterization::is_equal_to(
    grenade::common::PortData const& other) const
{
	return weights ==
	       static_cast<UncalibratedSynapse::ParameterSpace::Parameterization const&>(other).weights;
}

std::ostream& UncalibratedSynapse::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	ios << hate::Indentation("\t");
	ios << hate::join(weights.begin(), weights.end(), "\n");
	ios << hate::Indentation() << "\n)";
	return os;
}


UncalibratedSynapse::ParameterSpace::ParameterSpace(std::vector<Weight> max_weights) :
    max_weights(max_weights)
{
}

size_t UncalibratedSynapse::ParameterSpace::size() const
{
	return max_weights.size();
}

bool UncalibratedSynapse::ParameterSpace::valid(
    size_t input_port_on_synapse,
    grenade::common::Projection::Synapse::ParameterSpace::Parameterization const& parameterization)
    const
{
	if (input_port_on_synapse != 1) {
		return false;
	}
	if (auto const ptr = dynamic_cast<Parameterization const*>(&parameterization); ptr) {
		if (max_weights.size() != ptr->weights.size()) {
			return false;
		}
		for (size_t i = 0; i < max_weights.size(); ++i) {
			if (ptr->weights.at(i) > max_weights.at(i)) {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::unique_ptr<grenade::common::Projection::Synapse::ParameterSpace>
UncalibratedSynapse::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	std::vector<Weight> section_max_weights;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	for (auto const& element : sequence.get_elements()) {
		section_max_weights.push_back(max_weights.at(element.value.at(0)));
	}
	return std::make_unique<ParameterSpace>(std::move(section_max_weights));
}

std::unique_ptr<grenade::common::Projection::Synapse::ParameterSpace>
UncalibratedSynapse::ParameterSpace::copy() const
{
	return std::make_unique<UncalibratedSynapse::ParameterSpace>(*this);
}

std::unique_ptr<grenade::common::Projection::Synapse::ParameterSpace>
UncalibratedSynapse::ParameterSpace::move()
{
	return std::make_unique<UncalibratedSynapse::ParameterSpace>(std::move(*this));
}

bool UncalibratedSynapse::ParameterSpace::is_equal_to(
    grenade::common::Projection::Synapse::ParameterSpace const& other) const
{
	return max_weights ==
	       static_cast<UncalibratedSynapse::ParameterSpace const&>(other).max_weights;
}

std::ostream& UncalibratedSynapse::ParameterSpace::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "ParameterSpace(\n";
	ios << hate::Indentation("\t");
	ios << hate::join(max_weights.begin(), max_weights.end(), "\n");
	ios << hate::Indentation() << "\n)";
	return os;
}


bool UncalibratedSynapse::requires_time_domain() const
{
	return true;
}

bool UncalibratedSynapse::is_partitionable() const
{
	return true;
}

bool UncalibratedSynapse::valid(
    grenade::common::Projection::Synapse::ParameterSpace const& parameter_space) const
{
	return dynamic_cast<ParameterSpace const*>(&parameter_space) != nullptr;
}

bool UncalibratedSynapse::valid(size_t, grenade::common::Projection::Synapse::Dynamics const&) const
{
	return false;
}

bool UncalibratedSynapse::valid(
    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const
{
	return dynamic_cast<ClockCycleTimeDomainRuntimes const*>(&time_domain_runtimes) != nullptr;
}

UncalibratedSynapse::Ports UncalibratedSynapse::get_input_ports() const
{
	Ports ret;
	ret.projection.push_back(grenade::common::Vertex::Port(
	    Spike(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence()));
	ret.synapse_parameterization.push_back(grenade::common::Vertex::Port(
	    abstract::Weight(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence()));
	return ret;
}

UncalibratedSynapse::Ports UncalibratedSynapse::get_output_ports() const
{
	Ports ret;
	ret.projection.push_back(grenade::common::Vertex::Port(
	    SynapticInput(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence()));
	ret.synapse_parameterization.push_back(grenade::common::Vertex::Port(
	    abstract::SynapseObservable(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence()));
	return ret;
}

std::unique_ptr<grenade::common::Projection::Synapse> UncalibratedSynapse::copy() const
{
	return std::make_unique<UncalibratedSynapse>(*this);
}

std::unique_ptr<grenade::common::Projection::Synapse> UncalibratedSynapse::move()
{
	return std::make_unique<UncalibratedSynapse>(std::move(*this));
}

bool UncalibratedSynapse::is_equal_to(grenade::common::Projection::Synapse const&) const
{
	return true;
}

std::ostream& UncalibratedSynapse::print(std::ostream& os) const
{
	os << "UncalibratedSynapse()";
	return os;
}

} // namespace grenade::vx::network::abstract
