#include "grenade/vx/network/abstract/multicompartment_mechanism/capacitance.h"

namespace grenade::vx::network {


// Constructor MechanismCapacitance
MechanismCapacitance::MechanismCapacitance(ParameterSpace const& parameter_space_in) :
    parameter_space(parameter_space_in)
{
	if (!parameter_space.capacitance_interval.contains(
	        parameter_space.parameterization.capacitance)) {
		throw std::invalid_argument("Non valid Parameterization");
	}
}

// Check for Conflict with itself when placed on Compartment
bool MechanismCapacitance::conflict(Mechanism const& other) const
{
	return (typeid(*this) == typeid(other));
}

// Return HardwareRessource Requirements
HardwareResourcesWithConstraints MechanismCapacitance::get_hardware(
    CompartmentOnNeuron const&, Environment const&) const
{
	double capacity_convert = 5; // TO-DO
	HardwareResourcesWithConstraints resources_with_constraints;
	std::vector<common::detail::PropertyHolder<HardwareResource>> resource_list;

	double capacitance_model = parameter_space.capacitance_interval.get_upper();
	int num_of_hardware_resources;
	// Round up
	if (fmod(capacitance_model, capacity_convert) == 0) {
		num_of_hardware_resources = capacitance_model / capacity_convert;
	} else {
		num_of_hardware_resources = (capacitance_model / capacity_convert) + 1;
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
// Print-Dummy
std::ostream& MechanismCapacitance::print(std::ostream& os) const
{
	return os;
}

// Validate
bool MechanismCapacitance::valid() const
{
	return parameter_space.valid();
}

// Equality-Operator and Inequality-Operator
bool MechanismCapacitance::is_equal_to(Mechanism const& other) const
{
	MechanismCapacitance const& other_mechanism = static_cast<MechanismCapacitance const&>(other);

	// Check if Content is equal
	return (
	    parameter_space == other_mechanism.parameter_space &&
	    parameter_space.parameterization == other_mechanism.parameter_space.parameterization);
}

// Contructor ParameterSpace
MechanismCapacitance::ParameterSpace::ParameterSpace(
    ParameterInterval<double> const& parameter_interval_in,
    Parameterization const& parameterization_in) :
    capacitance_interval(parameter_interval_in), parameterization(parameterization_in)
{}

// Constructor Parameterization
MechanismCapacitance::ParameterSpace::Parameterization::Parameterization(double const& value) :
    capacitance(value)
{}

// MechanismCapacitance: Check if Parameterization is within ParameterSpace
bool MechanismCapacitance::ParameterSpace::valid() const
{
	return (capacitance_interval.contains(parameterization.capacitance));
}


} // namespace grenade::vx::network