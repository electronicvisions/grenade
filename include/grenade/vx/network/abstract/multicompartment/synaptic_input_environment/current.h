#pragma once

#include "grenade/vx/network/abstract/multicompartment/synaptic_input_environment.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct SYMBOL_VISIBLE GENPYBIND(visible) SynapticInputEnvironmentCurrent
    : public SynapticInputEnvironment
{
	SynapticInputEnvironmentCurrent() = default;
	SynapticInputEnvironmentCurrent(bool exitatory, int number);
	SynapticInputEnvironmentCurrent(bool exitatory, NumberTopBottom numbers);

	// Property Methods
	std::unique_ptr<SynapticInputEnvironment> copy() const;
	std::unique_ptr<SynapticInputEnvironment> move();

protected:
	std::ostream& print(std::ostream& os) const;
	bool is_equal_to(SynapticInputEnvironmentCurrent const& other) const;
};

} // namespace abstract
} // namespace grenade::vx::network