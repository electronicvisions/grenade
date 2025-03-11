#include "grenade/vx/network/abstract/multicompartment/mechanism/capacitance.h"
#include "hate/type_index.h"

namespace grenade::vx::network::abstract {

MechanismCapacitance::ParameterSpace::ParameterSpace(
    ParameterInterval<double> const& parameter_interval_in) :
    capacitance_interval(parameter_interval_in)
{
}

MechanismCapacitance::ParameterSpace::Parameterization::Parameterization(double const& value) :
    capacitance(value)
{
}

bool MechanismCapacitance::ParameterSpace::valid(
    Mechanism::ParameterSpace::Parameterization const& parameterization) const
{
	auto* cast_parameterization = dynamic_cast<Parameterization const*>(&parameterization);
	if (!cast_parameterization) {
		return false;
	}

	return capacitance_interval.contains(cast_parameterization->capacitance);
}

// Property methods Parameterization
std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismCapacitance::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<MechanismCapacitance::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismCapacitance::ParameterSpace::Parameterization::move()
{
	return std::make_unique<MechanismCapacitance::ParameterSpace::Parameterization>(
	    std::move(*this));
}

bool MechanismCapacitance::ParameterSpace::Parameterization::is_equal_to(
    Mechanism::ParameterSpace::Parameterization const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismCapacitance::ParameterSpace::Parameterization*>(&other);

	if (!other_cast) {
		return false;
	}
	return (capacitance == other_cast->capacitance);
}

std::ostream& MechanismCapacitance::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	os << "Parameterization(\n";
	os << "\tCapacitance: " << capacitance;
	os << "\n)";
	return os;
}

// Property methods ParameterSpace
std::unique_ptr<Mechanism::ParameterSpace> MechanismCapacitance::ParameterSpace::copy() const
{
	return std::make_unique<MechanismCapacitance::ParameterSpace>(*this);
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismCapacitance::ParameterSpace::move()
{
	return std::make_unique<MechanismCapacitance::ParameterSpace>(std::move(*this));
}

bool MechanismCapacitance::ParameterSpace::is_equal_to(Mechanism::ParameterSpace const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismCapacitance::ParameterSpace*>(&other);

	if (!other_cast) {
		return false;
	}
	return (capacitance_interval == other_cast->capacitance_interval);
}

std::ostream& MechanismCapacitance::ParameterSpace::print(std::ostream& os) const
{
	os << "Parameter-Space(\n";
	os << "\tCapacitance: " << capacitance_interval;
	os << "\n)";
	return os;
	return os;
}

// Check for Conflict with itself when placed on Compartment
bool MechanismCapacitance::conflict(Mechanism const& other) const
{
	return (typeid(*this) == typeid(other));
}

bool MechanismCapacitance::valid(Mechanism::ParameterSpace const&) const
{
	return true;
}

// Return HardwareRessource Requirements
HardwareResourcesWithConstraints MechanismCapacitance::get_hardware(
    CompartmentOnNeuron const&,
    Mechanism::ParameterSpace const& mechanism_parameter_space,
    Environment const&) const
{
	const auto* parameter_space =
	    dynamic_cast<const MechanismCapacitance::ParameterSpace*>(&mechanism_parameter_space);

	if (!parameter_space) {
		throw("Could not cast mechanism parameter space to capacitance parameter space.");
	}

	double capacity_convert = 5; // TO-DO
	HardwareResourcesWithConstraints resources_with_constraints;
	std::vector<dapr::PropertyHolder<HardwareResource>> resource_list;

	double capacitance_model = parameter_space->capacitance_interval.get_upper();
	int num_of_hardware_resources;
	// Round up
	if (fmod(capacitance_model, capacity_convert) == 0) {
		num_of_hardware_resources = capacitance_model / capacity_convert;
	} else {
		num_of_hardware_resources = (capacitance_model / capacity_convert) + 1;
	}

	// Always request one neuron circuit instead of none
	if (num_of_hardware_resources == 0) {
		num_of_hardware_resources = 1;
	}

	// Push Number of Required Hardware Ressources into Resource List
	for (int i = 0; i < num_of_hardware_resources; i++) {
		resource_list.push_back(HardwareResourceCapacity());
	}
	resources_with_constraints.resources = resource_list;
	return resources_with_constraints;
}

// Copy
std::unique_ptr<Mechanism> MechanismCapacitance::copy() const
{
	return std::make_unique<MechanismCapacitance>(*this);
}

// Move
std::unique_ptr<Mechanism> MechanismCapacitance::move()
{
	return std::make_unique<MechanismCapacitance>(std::move(*this));
}

// Print
std::ostream& MechanismCapacitance::print(std::ostream& os) const
{
	os << "MechanismCapacitance";
	return os;
}

// Equality-Operator and Inequality-Operator
bool MechanismCapacitance::is_equal_to(Mechanism const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismCapacitance*>(&other);

	if (!other_cast) {
		return false;
	}
	return true;
}


} // namespace grenade::vx::network::abstract