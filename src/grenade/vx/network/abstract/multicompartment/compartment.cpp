#include "grenade/vx/network/abstract/multicompartment/compartment.h"


namespace grenade::vx::network::abstract {

bool Compartment::ParameterSpace::valid(Parameterization const& parameterization) const
{
	for (auto [mechanism, mechanism_parameterization] : parameterization.mechanisms) {
		if (!mechanisms.get(mechanism).valid(mechanism_parameterization)) {
			return false;
		}
	}
	return true;
}

MechanismOnCompartment Compartment::Mechanisms::insert(Mechanism const& value)
{
	for (auto const& [mechanism_key, other_mechanism] : *this) {
		if (value.conflict(other_mechanism)) {
			std::stringstream ss;
			ss << "Conflicts with " << mechanism_key << ": " << other_mechanism;
			throw std::invalid_argument(ss.str());
		}
	}
	return AutoKeyMap::insert(value);
}

MechanismOnCompartment Compartment::Mechanisms::insert(Mechanism&& value)
{
	for (auto const& [mechanism_key, other_mechanism] : *this) {
		if (value.conflict(other_mechanism)) {
			std::stringstream ss;
			ss << "Conflicts with " << mechanism_key << ": " << other_mechanism;
			throw std::invalid_argument(ss.str());
		}
	}
	return AutoKeyMap::insert(std::move(value));
}

void Compartment::Mechanisms::set(MechanismOnCompartment const& key, Mechanism const& value)
{
	for (auto const& [mechanism_key, other_mechanism] : *this) {
		if (mechanism_key != key && value.conflict(other_mechanism)) {
			std::stringstream ss;
			ss << "Conflicts with " << mechanism_key << ": " << other_mechanism;
			throw std::invalid_argument(ss.str());
		}
	}
	AutoKeyMap::set(key, value);
}

void Compartment::Mechanisms::set(MechanismOnCompartment const& key, Mechanism&& value)
{
	for (auto const& [mechanism_key, other_mechanism] : *this) {
		if (mechanism_key != key && value.conflict(other_mechanism)) {
			std::stringstream ss;
			ss << "Conflicts with " << mechanism_key << ": " << other_mechanism;
			throw std::invalid_argument(ss.str());
		}
	}
	AutoKeyMap::set(key, std::move(value));
}

// Return HardwareRessource Requirements
std::map<MechanismOnCompartment, HardwareConstraints> Compartment::get_hardware(
    CompartmentOnNeuron const& compartment,
    Compartment::ParameterSpace const& parameter_space,
    Environment const& environment) const
{
	std::map<MechanismOnCompartment, HardwareConstraints> hardware_map;
	for (auto const& [key, value] : mechanisms) {
		hardware_map.emplace(
		    key, value.get_hardware(compartment, parameter_space.mechanisms.get(key), environment));
	}
	return hardware_map;
}

bool Compartment::valid(ParameterSpace const& parameter_space) const
{
	for (auto [mechanism, mechanism_parameter_space] : parameter_space.mechanisms) {
		if (!mechanisms.get(mechanism).valid(mechanism_parameter_space)) {
			return false;
		}
	}
	return true;
}

// Operators
bool Compartment::is_equal_to(Compartment const& other) const
{
	return (mechanisms == other.mechanisms);
}

// Property Methods
std::unique_ptr<Compartment> Compartment::copy() const
{
	return std::make_unique<Compartment>(*this);
}
std::unique_ptr<Compartment> Compartment::move()
{
	return std::make_unique<Compartment>(std::move(*this));
}
std::ostream& Compartment::print(std::ostream& os) const
{
	os << "Compartmentm_mechanisms: (";
	for (auto const& [_, mechanism] : mechanisms) {
		os << mechanism;
	}
	return os << ")";
}

} // namespace grenade::vx::network::abstract