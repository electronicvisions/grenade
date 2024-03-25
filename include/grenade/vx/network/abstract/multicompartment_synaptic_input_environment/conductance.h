#pragma once

#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment.h"

namespace grenade::vx::network {

struct SYMBOL_VISIBLE SynapticInputEnvironmentConductance : public SynapticInputEnvironment
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

} // namespace grenade::vx::network
