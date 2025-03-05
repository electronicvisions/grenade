#pragma once

#include "dapr/property.h"
#include "dapr/property_holder.h"
#include "grenade/common/detail/graph.h"
#include "grenade/common/graph.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource_with_constraint.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_on_compartment.h"
#include <map>
#include <memory>
#include <vector>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) Compartment : public dapr::Property<Compartment>
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
	std::map<MechanismOnCompartment, dapr::PropertyHolder<Mechanism>> m_mechanisms;
};

} // namespace abstract
} // namespace grenade::vx:network
