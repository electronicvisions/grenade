#include "grenade/vx/network/abstract/multicompartment_mechanism/synaptic_conductance.h"

namespace grenade::vx::network {

// Convert Number of Inputs to number of synaptical input circuits
int MechanismSynapticInputConductance::round(int i) const
{
	if (i % 256 == 0) {
		return (i / 256);
	} else {
		return ((i / 256) + 1);
	}
}

// Constructor MechanismSynapticInputConductance
MechanismSynapticInputConductance::MechanismSynapticInputConductance(
    ParameterSpace const& parameter_space_in) :
    parameter_space(parameter_space_in)
{
	if (!parameter_space.conductance_interval.contains(
	        parameter_space.parameterization.conductance) ||
	    !parameter_space.potential_interval.contains(parameter_space.parameterization.potential) ||
	    !parameter_space.time_constant_interval.contains(
	        parameter_space.parameterization.time_constant)) {
		throw std::invalid_argument("Non valid Parameterization");
	}
}

// Check for Conflict with itself when placed on Compartment
bool MechanismSynapticInputConductance::conflict(Mechanism const& /*other*/) const
{
	return false;
}

// Return HardwareRessource Requirements
HardwareResourcesWithConstraints MechanismSynapticInputConductance::get_hardware(
    CompartmentOnNeuron const& compartment, Environment const& environment) const
{
	// Return Object and Input
	HardwareResourcesWithConstraints resources_with_constraints;
	std::vector<common::detail::PropertyHolder<HardwareResource>> resource_list;
	std::vector<common::detail::PropertyHolder<HardwareConstraint>> constraint_list;
	if (environment.synaptic_connections.find(compartment) ==
	    environment.synaptic_connections.end()) {
		throw std::invalid_argument(" No information about this compartment in environment");
	}
	std::vector<common::detail::PropertyHolder<SynapticInputEnvironment>> synaptic_inputs =
	    environment.synaptic_connections.at(compartment);

	// Loop over all Synaptic Inputs of Compartment
	for (auto const& i : synaptic_inputs) {
		if (typeid(*i) != typeid(SynapticInputEnvironmentConductance)) {
			continue;
		}
		// Calculate Number of Synaptic Circuits required
		int number_of_inputs = round((*i).number_of_inputs.number_total);

		// Minimal Numbers in Top and Bottom Row
		if ((*i).number_of_inputs.number_top != 0 || (*i).number_of_inputs.number_bottom != 0) {
			int number_of_inputs_top = round((*i).number_of_inputs.number_top);
			int number_of_inputs_bottom = round((*i).number_of_inputs.number_bottom);

			if ((*i).exitatory) {
				// Add Exitatory Constraint
				bool in_list = false;
				for (auto j : constraint_list) {
					if (typeid((*j).resource) == typeid(HardwareResourceSynapticInputExitatory)) {
						(*j).numbers.number_bottom += number_of_inputs_bottom;
						(*j).numbers.number_top += number_of_inputs_top;
						(*j).numbers.number_total += number_of_inputs;
						in_list = true;
						break;
					}
				}
				if (!in_list) {
					HardwareConstraint constraint;
					constraint.resource = HardwareResourceSynapticInputExitatory();
					constraint.numbers.number_bottom += number_of_inputs_bottom;
					constraint.numbers.number_top += number_of_inputs_top;
					constraint.numbers.number_total += number_of_inputs;
					constraint_list.push_back(constraint);
				}


			} else {
				// Add Inhibitory Constraint
				bool in_list = false;
				for (auto j : constraint_list) {
					if (typeid((*j).resource) == typeid(HardwareResourceSynapticInputInhibitory)) {
						(*j).numbers.number_bottom += number_of_inputs_bottom;
						(*j).numbers.number_top += number_of_inputs_top;
						(*j).numbers.number_total += number_of_inputs;
						in_list = true;
						break;
					}
				}
				if (!in_list) {
					HardwareConstraint constraint;
					constraint.resource = HardwareResourceSynapticInputInhibitory();
					constraint.numbers.number_bottom += number_of_inputs_bottom;
					constraint.numbers.number_top += number_of_inputs_top;
					constraint.numbers.number_total += number_of_inputs;
					constraint_list.push_back(constraint);
				}
			}
		}


		// Add HardwareResource to Vector
		for (int j = 0; j < number_of_inputs; j++) {
			if ((*i).exitatory) {
				resource_list.push_back(HardwareResourceSynapticInputExitatory());
			} else {
				resource_list.push_back(HardwareResourceSynapticInputInhibitory());
			}
		}
	}

	// Return Resources and Constraints
	resources_with_constraints.resources = resource_list;
	resources_with_constraints.constraints = constraint_list;
	return resources_with_constraints;
}

// Copy
std::unique_ptr<Mechanism> MechanismSynapticInputConductance::copy() const
{
	return std::make_unique<MechanismSynapticInputConductance>(*this);
}
// Move
std::unique_ptr<Mechanism> MechanismSynapticInputConductance::move()
{
	return std::make_unique<MechanismSynapticInputConductance>(std::move(*this));
}
// Print-Dummy
std::ostream& MechanismSynapticInputConductance::print(std::ostream& os) const
{
	return os;
}

// Validate
bool MechanismSynapticInputConductance::valid() const
{
	return parameter_space.valid();
}

// Equality-Operator and Inequality-Operator
bool MechanismSynapticInputConductance::is_equal_to(Mechanism const& other) const
{
	MechanismSynapticInputConductance const& other_mechanism =
	    static_cast<MechanismSynapticInputConductance const&>(other);
	// Check if Content is equal
	return (
	    parameter_space == other_mechanism.parameter_space &&
	    parameter_space.parameterization == other_mechanism.parameter_space.parameterization);
}

// Contructor ParameterSpace
MechanismSynapticInputConductance::ParameterSpace::ParameterSpace(
    ParameterInterval<double> const& interval_conductance,
    ParameterInterval<double> const& interval_potential,
    ParameterInterval<double> const& interval_time_constant,
    Parameterization const& parameterization_in) :
    conductance_interval(interval_conductance),
    potential_interval(interval_potential),
    time_constant_interval(interval_time_constant),
    parameterization(parameterization_in)
{}

// Constructor Parameterization
MechanismSynapticInputConductance::ParameterSpace::Parameterization::Parameterization(
    double const& conductance_in, double const& potential_in, double const& time_constant_in) :
    conductance(conductance_in), potential(potential_in), time_constant(time_constant_in)
{}

// MechanismSynapticInputConductance: Check if Parameterization is within ParameterSpace
bool MechanismSynapticInputConductance::ParameterSpace::valid() const
{
	return (
	    conductance_interval.contains(parameterization.conductance) &&
	    potential_interval.contains(parameterization.potential) &&
	    time_constant_interval.contains(parameterization.time_constant));
}

} // namespace grenade::vx::network