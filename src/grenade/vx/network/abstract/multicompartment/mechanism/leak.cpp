#include "grenade/vx/network/abstract/multicompartment/mechanism/leak.h"
#include "hate/type_index.h"

namespace grenade::vx::network::abstract {

MechanismLeak::ParameterSpace::ParameterSpace(
    ParameterInterval<double> const& parameter_interval_conductance,
    ParameterInterval<double> const& parameter_interval_potential) :
    conductance_interval(parameter_interval_conductance),
    potential_interval(parameter_interval_potential)
{
}

MechanismLeak::ParameterSpace::Parameterization::Parameterization(
    double const& value_conductance, double const& value_potential) :
    conductance(value_conductance), potential(value_potential)
{
}

bool MechanismLeak::ParameterSpace::valid(
    Mechanism::ParameterSpace::Parameterization const& parameterization) const
{
	auto* cast_parameterization = dynamic_cast<Parameterization const*>(&parameterization);
	if (!cast_parameterization) {
		return false;
	}

	return conductance_interval.contains(cast_parameterization->conductance) &&
	       potential_interval.contains(cast_parameterization->potential);
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismLeak::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<MechanismLeak::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismLeak::ParameterSpace::Parameterization::move()
{
	return std::make_unique<MechanismLeak::ParameterSpace::Parameterization>(std::move(*this));
}

bool MechanismLeak::ParameterSpace::Parameterization::is_equal_to(
    Mechanism::ParameterSpace::Parameterization const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismLeak::ParameterSpace::Parameterization*>(&other);

	if (!other_cast) {
		return false;
	}
	return (conductance == other_cast->conductance) && (potential == other_cast->potential);
}

std::ostream& MechanismLeak::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	os << "Parameterization(\n"
	   << "\n\tLeak conductance: " << conductance << "\n\t Leak Potential: " << potential << "\n)";
	return os;
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismLeak::ParameterSpace::copy() const
{
	return std::make_unique<MechanismLeak::ParameterSpace>(*this);
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismLeak::ParameterSpace::move()
{
	return std::make_unique<MechanismLeak::ParameterSpace>(std::move(*this));
}

bool MechanismLeak::ParameterSpace::is_equal_to(Mechanism::ParameterSpace const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismLeak::ParameterSpace*>(&other);

	if (!other_cast) {
		return false;
	}
	return (conductance_interval == other_cast->conductance_interval) &&
	       (potential_interval == other_cast->potential_interval);
}

std::ostream& MechanismLeak::ParameterSpace::print(std::ostream& os) const
{
	os << "Parameter-Space(\n"
	   << "\n\tLeak Conductance: " << conductance_interval
	   << "\n\tLeak Potential: " << potential_interval << "\n)";
	return os;
}

bool MechanismLeak::conflict(Mechanism const& other) const
{
	return (typeid(*this) == typeid(other));
}

bool MechanismLeak::valid(Mechanism::ParameterSpace const&) const
{
	return true;
}

HardwareResourcesWithConstraints MechanismLeak::get_hardware(
    CompartmentOnNeuron const&,
    Mechanism::ParameterSpace const& mechanism_parameter_space,
    Environment const&) const
{
	const auto* parameter_space =
	    dynamic_cast<const MechanismLeak::ParameterSpace*>(&mechanism_parameter_space);

	if (!parameter_space) {
		throw("Given parameter space does not correspond to a leak-mechanism.");
	}

	HardwareResourcesWithConstraints resources_with_constraints;

	// Leak mechanism requires a single neuron circuit. Usecases having a higher conductance with
	// multiple neuron circuits is currently not supported.
	resources_with_constraints.resources.push_back(HardwareResourceLeak());
	return resources_with_constraints;
}

std::unique_ptr<Mechanism> MechanismLeak::copy() const
{
	return std::make_unique<MechanismLeak>(*this);
}

std::unique_ptr<Mechanism> MechanismLeak::move()
{
	return std::make_unique<MechanismLeak>(std::move(*this));
}

std::ostream& MechanismLeak::print(std::ostream& os) const
{
	os << "MechanismLeak";
	return os;
}

bool MechanismLeak::is_equal_to(Mechanism const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismLeak*>(&other);

	if (!other_cast) {
		return false;
	}
	return true;
}


} // namespace grenade::vx::network::abstract