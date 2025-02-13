#include "grenade/vx/network/abstract/multicompartment_mechanism/synaptic_current.h"

namespace grenade::vx::network::abstract {

int MechanismSynapticInputCurrent::round(int i) const
{
	if (i % 256 == 0) {
		return (i / 256);
	} else {
		return ((i / 256) + 1);
	}
}

// Constructor MechanismSynapticInputCurrent
MechanismSynapticInputCurrent::MechanismSynapticInputCurrent(
    ParameterSpace const& parameter_space_in) :
    parameter_space(parameter_space_in)
{
	if (!parameter_space.current_interval.contains(parameter_space.parameterization.current) ||
	    !parameter_space.time_constant_interval.contains(
	        parameter_space.parameterization.time_constant)) {
		throw std::invalid_argument("Non valid Parameterization");
	}
}


// Check for Conflict with itself when placed on Compartment
bool MechanismSynapticInputCurrent::conflict(Mechanism const& /*other*/) const
{
	return false;
}

// Return HardwareRessource Requirements
HardwareResourcesWithConstraints MechanismSynapticInputCurrent::get_hardware(
    CompartmentOnNeuron const& compartment, Environment const& environment) const
{
	// Return Object and Input
	HardwareResourcesWithConstraints resources_with_constraints;
	std::vector<common::PropertyHolder<HardwareResource>> resource_list;
	std::vector<common::PropertyHolder<HardwareConstraint>> constraint_list;
	if (environment.synaptic_connections.find(compartment) ==
	    environment.synaptic_connections.end()) {
		throw std::invalid_argument(" No information about this compartment in environment");
	}
	std::vector<common::PropertyHolder<SynapticInputEnvironment>> synaptic_inputs =
	    environment.synaptic_connections.at(compartment);

	// Loop over all Synaptic Inputs of Compartment
	for (auto i : synaptic_inputs) {
		if (typeid(*i) != typeid(SynapticInputEnvironmentCurrent)) {
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
std::unique_ptr<Mechanism> MechanismSynapticInputCurrent::copy() const
{
	return std::make_unique<MechanismSynapticInputCurrent>(*this);
}
// Move
std::unique_ptr<Mechanism> MechanismSynapticInputCurrent::move()
{
	return std::make_unique<MechanismSynapticInputCurrent>(std::move(*this));
}
// Print-Dummy
std::ostream& MechanismSynapticInputCurrent::print(std::ostream& os) const
{
	return os;
}

// Validate
bool MechanismSynapticInputCurrent::valid() const
{
	return parameter_space.valid();
}

// Contructor ParameterSpace
MechanismSynapticInputCurrent::ParameterSpace::ParameterSpace(
    ParameterInterval<double> const& interval_current_in,
    ParameterInterval<double> const& interval_time_constant_in,
    Parameterization const& parameterization_in) :
    current_interval(interval_current_in),
    time_constant_interval(interval_time_constant_in),
    parameterization(parameterization_in)
{}

// Constructor Parameterization
MechanismSynapticInputCurrent::ParameterSpace::Parameterization::Parameterization(
    double const& current_in, double const& time_constant_in) :
    current(current_in), time_constant(time_constant_in)
{}

// MechanismSynapticInputCurrent: Check if Parameterization is within ParameterSpace
bool MechanismSynapticInputCurrent::ParameterSpace::valid() const
{
	return (
	    current_interval.contains(parameterization.current) &&
	    time_constant_interval.contains(parameterization.time_constant));
}

bool MechanismSynapticInputCurrent::is_equal_to(Mechanism const& other) const
{
	MechanismSynapticInputCurrent const& temp_mechanism =
	    static_cast<MechanismSynapticInputCurrent const&>(other);
	return (
	    parameter_space == temp_mechanism.parameter_space &&
	    parameter_space.parameterization == temp_mechanism.parameter_space.parameterization);
}


} // namespace grenade::vx::network::abstract