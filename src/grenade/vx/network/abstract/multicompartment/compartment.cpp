#include "grenade/vx/network/abstract/multicompartment/compartment.h"

#include "dapr/property_holder.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_environment.h"
#include "hate/indent.h"

namespace grenade::vx::network::abstract {

size_t Compartment::ParameterSpace::Parameterization::size() const
{
	if (mechanisms.empty()) {
		return 0;
	}
	std::set<size_t> ret;
	for (auto const& [_, mechanism] : mechanisms) {
		ret.insert(mechanism.size());
	}
	if (ret.size() > 1) {
		throw std::runtime_error("Compartment parameterization features heterogeneous size.");
	}
	return *ret.begin();
}

Compartment::ParameterSpace::Parameterization
Compartment::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	Parameterization ret;
	for (auto const& [mechanism_on_compartment, mechanism] : mechanisms) {
		ret.mechanisms.set(mechanism_on_compartment, *mechanism.get_section(sequence));
	}
	return ret;
}

std::ostream& operator<<(
    std::ostream& os, Compartment::ParameterSpace::Parameterization const& value)
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	ios << hate::Indentation("\t");
	for (auto const& [mechanism_on_compartment, mechanism] : value.mechanisms) {
		ios << mechanism_on_compartment << ": " << mechanism << "\n";
	}
	ios << hate::Indentation("\t");
	ios << ")";
	return os;
}


size_t Compartment::ParameterSpace::size() const
{
	if (mechanisms.empty()) {
		return 0;
	}
	std::set<size_t> ret;
	for (auto const& [_, mechanism] : mechanisms) {
		ret.insert(mechanism.size());
	}
	if (ret.size() > 1) {
		throw std::runtime_error("Compartment parameter space features heterogeneous size.");
	}
	return *ret.begin();
}

Compartment::ParameterSpace Compartment::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	ParameterSpace ret;
	for (auto const& [mechanism_on_compartment, mechanism] : mechanisms) {
		ret.mechanisms.set(mechanism_on_compartment, *mechanism.get_section(sequence));
	}
	return ret;
}

bool Compartment::ParameterSpace::valid(Parameterization const& parameterization) const
{
	if (parameterization.size() != size()) {
		return false;
	}
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

std::ostream& operator<<(std::ostream& os, Compartment::ParameterSpace const& value)
{
	hate::IndentingOstream ios(os);
	ios << "ParameterSpace(\n";
	ios << hate::Indentation("\t");
	for (auto const& [mechanism_on_compartment, mechanism] : value.mechanisms) {
		ios << mechanism_on_compartment << ": " << mechanism << "\n";
	}
	ios << hate::Indentation("\t");
	ios << ")";
	return os;
}


// Return HardwareRessource Requirements
std::map<MechanismOnCompartment, HardwareConstraints> Compartment::get_hardware(
    Compartment::ParameterSpace const& parameter_space,
    std::map<MechanismOnCompartment, dapr::PropertyHolder<MechanismEnvironment>> const& environment)
    const
{
	std::map<MechanismOnCompartment, HardwareConstraints> hardware_map;
	for (auto const& [key, value] : mechanisms) {
		MechanismEnvironment const* mechanism_environment = nullptr;
		if (environment.contains(key)) {
			mechanism_environment = &(*environment.at(key));
		}
		hardware_map.emplace(
		    key, value.get_hardware(parameter_space.mechanisms.get(key), mechanism_environment));
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