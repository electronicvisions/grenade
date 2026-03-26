#include "grenade/vx/network/abstract/multicompartment/mechanism/event_output.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/network/abstract/vertex_port_type/spike.h"
#include "haldls/vx/v3/cadc.h"
#include "hate/join.h"
#include "hate/type_index.h"

namespace grenade::vx::network::abstract {

MechanismEventOutput::ParameterSpace::ParameterSpace(size_t size) : m_size(size) {}

std::unique_ptr<Mechanism::ParameterSpace> MechanismEventOutput::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameterization.");
	}
	return std::make_unique<ParameterSpace>(sequence.size());
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismEventOutput::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameter space.");
	}
	return std::make_unique<Parameterization>(sequence.size());
}

size_t MechanismEventOutput::ParameterSpace::size() const
{
	return m_size;
}

MechanismEventOutput::ParameterSpace::Parameterization::Parameterization(size_t size) : m_size(size)
{
}

size_t MechanismEventOutput::ParameterSpace::Parameterization::size() const
{
	return m_size;
}

bool MechanismEventOutput::ParameterSpace::valid(
    Mechanism::ParameterSpace::Parameterization const& parameterization) const
{
	auto* cast_parameterization = dynamic_cast<Parameterization const*>(&parameterization);
	if (!cast_parameterization) {
		return false;
	}

	return size() == cast_parameterization->size();
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismEventOutput::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<MechanismEventOutput::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismEventOutput::ParameterSpace::Parameterization::move()
{
	return std::make_unique<MechanismEventOutput::ParameterSpace::Parameterization>(
	    std::move(*this));
}

bool MechanismEventOutput::ParameterSpace::Parameterization::is_equal_to(
    Mechanism::ParameterSpace::Parameterization const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismEventOutput::ParameterSpace::Parameterization*>(&other);

	if (!other_cast) {
		return false;
	}
	return m_size == other_cast->m_size;
}

std::ostream& MechanismEventOutput::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	os << "Parameterization(" << m_size << ")";
	return os;
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismEventOutput::ParameterSpace::copy() const
{
	return std::make_unique<MechanismEventOutput::ParameterSpace>(*this);
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismEventOutput::ParameterSpace::move()
{
	return std::make_unique<MechanismEventOutput::ParameterSpace>(std::move(*this));
}

bool MechanismEventOutput::ParameterSpace::is_equal_to(Mechanism::ParameterSpace const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismEventOutput::ParameterSpace*>(&other);

	if (!other_cast) {
		return false;
	}
	return m_size == other_cast->m_size;
}

std::ostream& MechanismEventOutput::ParameterSpace::print(std::ostream& os) const
{
	os << "ParameterSpace(" << m_size << ")";
	return os;
}

bool MechanismEventOutput::conflict(Mechanism const& other) const
{
	return (typeid(*this) == typeid(other));
}

bool MechanismEventOutput::valid(Mechanism::ParameterSpace const&) const
{
	return true;
}

std::vector<grenade::common::Vertex::Port> MechanismEventOutput::get_input_ports() const
{
	return {};
}

std::vector<grenade::common::Vertex::Port> MechanismEventOutput::get_output_ports() const
{
	return {grenade::common::Vertex::Port(
	    Spike(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence())};
}

HardwareConstraints MechanismEventOutput::get_hardware(
    Mechanism::ParameterSpace const& mechanism_parameter_space, MechanismEnvironment const*) const
{
	const auto* parameter_space =
	    dynamic_cast<const MechanismEventOutput::ParameterSpace*>(&mechanism_parameter_space);

	if (!parameter_space) {
		throw("Given parameter space does not correspond to a event-output-mechanism.");
	}

	HardwareConstraints constraints;

	// EventOutput mechanism requires a single neuron circuit.
	HardwareConstraint constraint;
	constraint.resource = HardwareResourceEventOutput();
	constraint.numbers.number_total = 1;
	constraints.push_back(std::move(constraint));
	return constraints;
}

std::unique_ptr<Mechanism> MechanismEventOutput::copy() const
{
	return std::make_unique<MechanismEventOutput>(*this);
}

std::unique_ptr<Mechanism> MechanismEventOutput::move()
{
	return std::make_unique<MechanismEventOutput>(std::move(*this));
}

std::ostream& MechanismEventOutput::print(std::ostream& os) const
{
	os << "MechanismEventOutput";
	return os;
}

bool MechanismEventOutput::is_equal_to(Mechanism const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismEventOutput*>(&other);

	if (!other_cast) {
		return false;
	}
	return true;
}


} // namespace grenade::vx::network::abstract