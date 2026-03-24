#include "grenade/vx/network/abstract/multicompartment/mechanism/capacitance.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/analog_readout.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/with_analog_readout.h"
#include "hate/join.h"
#include "hate/type_index.h"

namespace grenade::vx::network::abstract {

MechanismCapacitance::ParameterSpace::ParameterSpace(
    std::vector<ParameterInterval<ccalix::CapacitanceInFarad>> parameter_in) :
    capacitance(std::move(parameter_in))
{
}

MechanismCapacitance::ParameterSpace::Parameterization::Parameterization(
    std::vector<ccalix::CapacitanceInFarad> value) :
    capacitance(std::move(value))
{
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismCapacitance::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	ParameterSpace ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameterization.");
	}
	ret.capacitance.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.capacitance.push_back(capacitance.at(element.value.at(0)));
	}
	return std::make_unique<ParameterSpace>(std::move(ret));
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismCapacitance::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	Parameterization ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameter space.");
	}
	ret.capacitance.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.capacitance.push_back(capacitance.at(element.value.at(0)));
	}
	return std::make_unique<Parameterization>(std::move(ret));
}

size_t MechanismCapacitance::ParameterSpace::size() const
{
	return capacitance.size();
}

size_t MechanismCapacitance::ParameterSpace::Parameterization::size() const
{
	return capacitance.size();
}

bool MechanismCapacitance::ParameterSpace::valid(
    Mechanism::ParameterSpace::Parameterization const& parameterization) const
{
	if (parameterization.size() != size()) {
		return false;
	}
	auto* cast_parameterization = dynamic_cast<Parameterization const*>(&parameterization);
	if (!cast_parameterization) {
		return false;
	}

	for (size_t i = 0; i < size(); ++i) {
		if (!capacitance.at(i).contains(cast_parameterization->capacitance.at(i))) {
			return false;
		}
	}
	return true;
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
	os << "\tCapacitance: " << hate::join(capacitance, ", ");
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
	return (capacitance == other_cast->capacitance);
}

std::ostream& MechanismCapacitance::ParameterSpace::print(std::ostream& os) const
{
	os << "Parameter-Space(\n";
	os << "\tCapacitance: " << hate::join(capacitance, ", ");
	os << "\n)";
	return os;
	return os;
}

MechanismCapacitance::MechanismCapacitance(bool enable_analog_readout) :
    MechanismWithAnalogReadout(enable_analog_readout)
{
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
HardwareConstraints MechanismCapacitance::get_hardware(
    Mechanism::ParameterSpace const& mechanism_parameter_space, MechanismEnvironment const*) const
{
	const auto* parameter_space =
	    dynamic_cast<const MechanismCapacitance::ParameterSpace*>(&mechanism_parameter_space);

	if (!parameter_space) {
		throw("Could not cast mechanism parameter space to capacitance parameter space.");
	}

	ccalix::CapacitanceInFarad capacity_convert(
	    2.2e-12); // TODO: find better-matching minimal value including parasitic capacitances
	HardwareConstraints constraints;

	ccalix::CapacitanceInFarad capacitance_model(0.);
	for (auto const& capacitance : parameter_space->capacitance) {
		capacitance_model = std::max(capacitance_model, capacitance.get_upper());
	}
	int num_of_hardware_resources;
	// Round up
	if (fmod(capacitance_model.value(), capacity_convert.value()) == 0) {
		num_of_hardware_resources = capacitance_model.value() / capacity_convert.value();
	} else {
		num_of_hardware_resources = (capacitance_model.value() / capacity_convert.value()) + 1;
	}

	// Always request one neuron circuit instead of none
	if (num_of_hardware_resources == 0) {
		num_of_hardware_resources = 1;
	}

	// Push Number of Required Hardware Ressources into Resource List
	HardwareConstraint constraint;
	constraint.numbers.number_total = num_of_hardware_resources;
	constraint.resource = HardwareResourceCapacity();
	constraints.push_back(std::move(constraint));

	if (enable_analog_readout) {
		HardwareConstraint readout_constraint;
		readout_constraint.numbers.number_total = 1;
		readout_constraint.resource = HardwareResourceAnalogReadout();
		constraints.push_back(std::move(readout_constraint));
	}

	return constraints;
}

lola::vx::v3::AtomicNeuron::Readout::Source MechanismCapacitance::get_analog_readout_source() const
{
	return lola::vx::v3::AtomicNeuron::Readout::Source::membrane;
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
	os << "MechanismCapacitance(enable_analog_readout: " << enable_analog_readout << ")";
	return os;
}

// Equality-Operator and Inequality-Operator
bool MechanismCapacitance::is_equal_to(Mechanism const& other) const
{
	return enable_analog_readout ==
	       static_cast<MechanismCapacitance const&>(other).enable_analog_readout;
}


} // namespace grenade::vx::network::abstract
