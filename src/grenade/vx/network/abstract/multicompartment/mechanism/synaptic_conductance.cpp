#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_conductance.h"

namespace grenade::vx::network::abstract {

MechanismSynapticInputConductance::ParameterSpace::ParameterSpace(
    ParameterInterval<double> const& interval_conductance,
    ParameterInterval<double> const& interval_potential,
    ParameterInterval<double> const& interval_time_constant) :
    conductance_interval(interval_conductance),
    potential_interval(interval_potential),
    time_constant_interval(interval_time_constant)
{
}

MechanismSynapticInputConductance::ParameterSpace::Parameterization::Parameterization(
    double const& conductance_in, double const& potential_in, double const& time_constant_in) :
    conductance(conductance_in), potential(potential_in), time_constant(time_constant_in)
{
}

bool MechanismSynapticInputConductance::ParameterSpace::valid(
    Mechanism::ParameterSpace::Parameterization const& parameterization) const
{
	auto* cast_parameterization = dynamic_cast<Parameterization const*>(&parameterization);
	if (!cast_parameterization) {
		return false;
	}

	return conductance_interval.contains(cast_parameterization->conductance) &&
	       potential_interval.contains(cast_parameterization->potential) &&
	       time_constant_interval.contains(cast_parameterization->time_constant);
}

// Property methods Parameterization
std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismSynapticInputConductance::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<MechanismSynapticInputConductance::ParameterSpace::Parameterization>(
	    *this);
}
std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismSynapticInputConductance::ParameterSpace::Parameterization::move()
{
	return std::make_unique<MechanismSynapticInputConductance::ParameterSpace::Parameterization>(
	    std::move(*this));
}
bool MechanismSynapticInputConductance::ParameterSpace::Parameterization::is_equal_to(
    Mechanism::ParameterSpace::Parameterization const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismSynapticInputConductance::ParameterSpace::Parameterization*>(
	        &other);

	if (!other_cast) {
		return false;
	}
	return (
	    conductance == other_cast->conductance && potential == other_cast->potential &&
	    time_constant == other_cast->time_constant);
}
std::ostream& MechanismSynapticInputConductance::ParameterSpace::Parameterization::print(
    std::ostream& os) const
{
	os << "Parameterization(\n";
	os << "\tConductance: " << conductance;
	os << "\n\tPotential:" << potential;
	os << "\n\tTime-constant: " << time_constant;
	os << "\n)";
	return os;
}

// Property methods ParameterSpace
std::unique_ptr<Mechanism::ParameterSpace> MechanismSynapticInputConductance::ParameterSpace::copy()
    const
{
	return std::make_unique<MechanismSynapticInputConductance::ParameterSpace>(*this);
}
std::unique_ptr<Mechanism::ParameterSpace> MechanismSynapticInputConductance::ParameterSpace::move()
{
	return std::make_unique<MechanismSynapticInputConductance::ParameterSpace>(std::move(*this));
}
bool MechanismSynapticInputConductance::ParameterSpace::is_equal_to(
    Mechanism::ParameterSpace const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismSynapticInputConductance::ParameterSpace*>(&other);

	if (!other_cast) {
		return false;
	}
	return (
	    conductance_interval == other_cast->conductance_interval &&
	    potential_interval == other_cast->potential_interval &&
	    time_constant_interval == other_cast->time_constant_interval);
}
std::ostream& MechanismSynapticInputConductance::ParameterSpace::print(std::ostream& os) const
{
	os << "ParameterSpace(\n";
	os << "\tConductance: " << conductance_interval;
	os << "\n\tPotential:" << potential_interval;
	os << "\n\tTime-constant: " << time_constant_interval;
	os << "\n)";
	return os;
}


// Convert Number of Inputs to number of synaptical input circuits
int MechanismSynapticInputConductance::round(int i) const
{
	if (i % 256 == 0) {
		return (i / 256);
	} else {
		return ((i / 256) + 1);
	}
}

// Check for Conflict with itself when placed on Compartment
bool MechanismSynapticInputConductance::conflict(Mechanism const& /*other*/) const
{
	return false;
}

bool MechanismSynapticInputConductance::valid(Mechanism::ParameterSpace const&) const
{
	return true;
}

// Return HardwareRessource Requirements
HardwareResourcesWithConstraints MechanismSynapticInputConductance::get_hardware(
    CompartmentOnNeuron const& compartment,
    Mechanism::ParameterSpace const& mechanism_parameter_space,
    Environment const& environment) const
{
	const auto* parameter_space =
	    dynamic_cast<const MechanismSynapticInputConductance::ParameterSpace*>(
	        &mechanism_parameter_space);

	if (!parameter_space) {
		throw("Could not cast mechanism parameter space to synaptic conductance parameter space.");
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
	for (auto const& i : synaptic_inputs) {
		if (typeid(*i) != typeid(SynapticInputEnvironmentConductance)) {
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
std::unique_ptr<Mechanism> MechanismSynapticInputConductance::copy() const
{
	return std::make_unique<MechanismSynapticInputConductance>(*this);
}
// Move
std::unique_ptr<Mechanism> MechanismSynapticInputConductance::move()
{
	return std::make_unique<MechanismSynapticInputConductance>(std::move(*this));
}
// Print
std::ostream& MechanismSynapticInputConductance::print(std::ostream& os) const
{
	os << "MechanismSynapticInputConductance\n";
	return os;
}

// Equality-Operator and Inequality-Operator
bool MechanismSynapticInputConductance::is_equal_to(Mechanism const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismSynapticInputConductance*>(&other);
	if (!other_cast) {
		return false;
	}
	return true;
}

} // namespace grenade::vx::network::abstract