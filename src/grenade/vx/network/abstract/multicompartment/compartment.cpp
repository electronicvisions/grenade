#include "grenade/vx/network/abstract/multicompartment/compartment.h"


namespace grenade::vx::network::abstract {


// Adds Mechanism to Compartment
MechanismOnCompartment Compartment::add(Mechanism const& mechanism)
{
	for (auto const& [mechanism_key, other_mechanism] : m_mechanisms) {
		if (mechanism.conflict(*other_mechanism)) {
			throw std::invalid_argument("Conflicting Mechanism");
		}
	}
	if (m_mechanism_key_counter == std::numeric_limits<int>::max()) {
		throw std::invalid_argument("Too many Mechanisms. Overflow of Key Counter");
	}
	MechanismOnCompartment mechanismKey(m_mechanism_key_counter++);
	m_mechanisms.emplace(mechanismKey, mechanism);
	return mechanismKey;
}

// Removes Mechanism from Compartment by Key
void Compartment::remove(MechanismOnCompartment const& descriptor)
{
	if (!m_mechanisms.contains(descriptor)) {
		throw std::invalid_argument("Mechanism not on Compartment");
	}
	m_mechanisms.erase(descriptor);
}

// Returns Mechanism of Compartment by Key
Mechanism const& Compartment::get(MechanismOnCompartment const& descriptor) const
{
	if (m_mechanisms.find(descriptor) == m_mechanisms.end()) {
		throw std::invalid_argument("Mechanism not on Compartment");
	}
	return *(m_mechanisms.at(descriptor));
}

// Changes Mechanism of Compartment by Key
void Compartment::set(MechanismOnCompartment const& descriptor, Mechanism const& mechanism)
{
	if (m_mechanisms.find(descriptor) == m_mechanisms.end()) {
		throw std::invalid_argument("Mechanism not on Compartment");
	}
	m_mechanisms[descriptor] = mechanism;
}

// Return HardwareRessource Requirements
std::map<MechanismOnCompartment, HardwareResourcesWithConstraints> Compartment::get_hardware(
    CompartmentOnNeuron const& compartment, Environment const& environment) const
{
	std::map<MechanismOnCompartment, HardwareResourcesWithConstraints> hardware_map;
	for (auto const& [key, value] : m_mechanisms) {
		hardware_map.emplace(key, value->get_hardware(compartment, environment));
	}
	return hardware_map;
}


// Operators
bool Compartment::is_equal_to(Compartment const& other) const
{
	return (m_mechanisms == other.m_mechanisms);
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
	for (auto const& [_, mechanism] : m_mechanisms) {
		os << *mechanism;
	}
	return os << ")";
}

bool Compartment::valid() const
{
	for (auto const& [_, mechanism] : m_mechanisms) {
		if ((*mechanism).valid()) {
			return false;
		}
	}
	return true;
}

} // namespace grenade::vx::network::abstract