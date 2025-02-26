#pragma once

#include "grenade/vx/network/abstract/detail/graph.h"
#include "grenade/vx/network/abstract/detail/property_holder.h"
#include "grenade/vx/network/abstract/graph.h"
#include "grenade/vx/network/abstract/multicompartment_hardware_resource_with_constraint.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism.h"
#include "grenade/vx/network/abstract/multicompartment_mechanism_on_compartment.h"
#include "grenade/vx/network/abstract/property.h"
#include <map>
#include <memory>
#include <vector>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {


struct SYMBOL_VISIBLE GENPYBIND(visible) Compartment : public Property<Compartment>
{
	virtual MechanismOnCompartment add(Mechanism const& mechanism);
	virtual void remove(MechanismOnCompartment const& descriptor);
	virtual Mechanism const& get(MechanismOnCompartment const& descriptor) const;
	virtual void set(MechanismOnCompartment const& descriptor, Mechanism const& mechanism);

	// Return HardwareRessource Requirements
	std::map<MechanismOnCompartment, HardwareResourcesWithConstraints> get_hardware(
	    CompartmentOnNeuron const& compartment, Environment const& environment) const;

	// Property Methods
	std::unique_ptr<Compartment> copy() const;
	std::unique_ptr<Compartment> move();

	/**
	 * Returns if all mechanisms on the compartment are valid.
	 */
	bool valid() const;

protected:
	bool is_equal_to(Compartment const& other) const;
	std::ostream& print(std::ostream& os) const;

private:
	size_t m_mechanism_key_counter = 0;
	// Map over all Mechanisms on a Compartment
	std::map<MechanismOnCompartment, detail::PropertyHolder<Mechanism>> m_mechanisms;
};

} // namespace network
} // namespace grenade::vx
