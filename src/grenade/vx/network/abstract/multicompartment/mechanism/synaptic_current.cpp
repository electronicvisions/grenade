#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_current.h"

namespace grenade::vx::network::abstract {

MechanismSynapticInputCurrent::ParameterSpace::ParameterSpace(
    ParameterInterval<double> const& interval_current_in,
    ParameterInterval<double> const& interval_time_constant_in) :
    current_interval(interval_current_in), time_constant_interval(interval_time_constant_in)
{
}

MechanismSynapticInputCurrent::ParameterSpace::Parameterization::Parameterization(
    double const& current_in, double const& time_constant_in) :
    current(current_in), time_constant(time_constant_in)
{
}

bool MechanismSynapticInputCurrent::ParameterSpace::valid(
    Mechanism::ParameterSpace::Parameterization const& parameterization) const
{
	auto* cast_parameterization = dynamic_cast<Parameterization const*>(&parameterization);
	if (!cast_parameterization) {
		return false;
	}

	return current_interval.contains(cast_parameterization->current) &&
	       time_constant_interval.contains(cast_parameterization->time_constant);
}

// Property methods Parameterization
std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismSynapticInputCurrent::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<MechanismSynapticInputCurrent::ParameterSpace::Parameterization>(*this);
}
std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismSynapticInputCurrent::ParameterSpace::Parameterization::move()
{
	return std::make_unique<MechanismSynapticInputCurrent::ParameterSpace::Parameterization>(
	    std::move(*this));
}
bool MechanismSynapticInputCurrent::ParameterSpace::Parameterization::is_equal_to(
    Mechanism::ParameterSpace::Parameterization const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismSynapticInputCurrent::ParameterSpace::Parameterization*>(
	        &other);

	if (!other_cast) {
		return false;
	}
	return (current == other_cast->current && time_constant == other_cast->time_constant);
}
std::ostream& MechanismSynapticInputCurrent::ParameterSpace::Parameterization::print(
    std::ostream& os) const
{
	os << "Parameterization(\n";
	os << "\tCurrent: " << current;
	os << "\n\tTime-constant: " << time_constant;
	os << "\n)";
	return os;
}

// Property methods ParameterSpace
std::unique_ptr<Mechanism::ParameterSpace> MechanismSynapticInputCurrent::ParameterSpace::copy()
    const
{
	return std::make_unique<MechanismSynapticInputCurrent::ParameterSpace>(*this);
}
std::unique_ptr<Mechanism::ParameterSpace> MechanismSynapticInputCurrent::ParameterSpace::move()
{
	return std::make_unique<MechanismSynapticInputCurrent::ParameterSpace>(std::move(*this));
}
bool MechanismSynapticInputCurrent::ParameterSpace::is_equal_to(
    Mechanism::ParameterSpace const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismSynapticInputCurrent::ParameterSpace*>(&other);

	if (!other_cast) {
		return false;
	}
	return (
	    current_interval == other_cast->current_interval &&
	    time_constant_interval == other_cast->time_constant_interval);
}
std::ostream& MechanismSynapticInputCurrent::ParameterSpace::print(std::ostream& os) const
{
	os << "ParameterSpace(\n";
	os << "\tCurrent: " << current_interval;
	os << "\n\tTime-constant: " << time_constant_interval;
	os << "\n)";
	return os;
}


int MechanismSynapticInputCurrent::round(int i) const
{
	if (i % 256 == 0) {
		return (i / 256);
	} else {
		return ((i / 256) + 1);
	}
}

// Check for Conflict with itself when placed on Compartment
bool MechanismSynapticInputCurrent::conflict(Mechanism const& /*other*/) const
{
	return false;
}

bool MechanismSynapticInputCurrent::valid(Mechanism::ParameterSpace const&) const
{
	return true;
}

// Return HardwareRessource Requirements
HardwareResourcesWithConstraints MechanismSynapticInputCurrent::get_hardware(
    CompartmentOnNeuron const& compartment,
    Mechanism::ParameterSpace const& mechanism_parameter_space,
    Environment const& environment) const
{
	const auto* parameter_space =
	    dynamic_cast<const MechanismSynapticInputCurrent::ParameterSpace*>(
	        &mechanism_parameter_space);

	if (!parameter_space) {
		throw("Could not cast mechanism parameter space to capacitance parameter space.");
	}

	// Return Object and Input
	HardwareResourcesWithConstraints resources_with_constraints;
	std::vector<dapr::PropertyHolder<HardwareResource>> resource_list;
	std::vector<dapr::PropertyHolder<HardwareConstraint>> constraint_list;

	std::vector<dapr::PropertyHolder<SynapticInputEnvironment>> synaptic_inputs =
	    environment.get(compartment);

	if (synaptic_inputs.size() == 0) {
		throw std::invalid_argument(" No information about this compartment in environment");
	}

	// Loop over all Synaptic Inputs of Compartment
	for (auto i : synaptic_inputs) {
		if (typeid(*i) != typeid(SynapticInputEnvironmentCurrent)) {
			continue;
		}
		// Calculate Number of Synaptic Circuits required
		int number_of_inputs = round((*i).number_of_inputs.number_total);

		// Always request one neuron circuit instead of none
		if (number_of_inputs == 0) {
			number_of_inputs = 1;
		}
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
// Print
std::ostream& MechanismSynapticInputCurrent::print(std::ostream& os) const
{
	os << "MechanismSynapticInputCurrent";
	return os;
}


bool MechanismSynapticInputCurrent::is_equal_to(Mechanism const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismSynapticInputCurrent*>(&other);

	if (!other_cast) {
		return false;
	}
	return true;
}


} // namespace grenade::vx::network::abstract