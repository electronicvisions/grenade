#pragma once
#include "dapr/property.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_environment.h"
#include "grenade/vx/network/abstract/multicompartment/top_bottom.h"
#include <utility>


namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

struct SYMBOL_VISIBLE GENPYBIND(visible) SynapticInputEnvironment : public MechanismEnvironment
{
	SynapticInputEnvironment() = default;
	/**
	 * Construct synaptic input environment.
	 * @param number Total number of circuits without specification of top/bottom constraints
	 */
	SynapticInputEnvironment(int number);
	SynapticInputEnvironment(NumberTopBottom numbers);

	NumberTopBottom number_of_inputs;

	// Property Methods
	virtual std::unique_ptr<MechanismEnvironment> copy() const;
	virtual std::unique_ptr<MechanismEnvironment> move();

protected:
	virtual std::ostream& print(std::ostream& os) const;
	virtual bool is_equal_to(MechanismEnvironment const& other) const;
};

} // abstract
} // namespace grenade::vx::network
