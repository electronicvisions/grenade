#pragma once

#include "grenade/vx/network/abstract/multicompartment/synaptic_input_environment.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct SYMBOL_VISIBLE GENPYBIND(visible) SynapticInputEnvironmentConductance
    : public SynapticInputEnvironment
{
	SynapticInputEnvironmentConductance() = default;
	SynapticInputEnvironmentConductance(bool exitatory, int number);
	SynapticInputEnvironmentConductance(bool exitatory, NumberTopBottom numbers);

	// Property Methods
	std::unique_ptr<SynapticInputEnvironment> copy() const;
	std::unique_ptr<SynapticInputEnvironment> move();

protected:
	std::ostream& print(std::ostream& os) const;
	bool is_equal_to(SynapticInputEnvironmentConductance const& other) const;
};

} // namespace abstract
} // namespace grenade::vx::network
