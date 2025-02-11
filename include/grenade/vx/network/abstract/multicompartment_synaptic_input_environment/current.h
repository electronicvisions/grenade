#pragma once

#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment.h"

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE SynapticInputEnvironmentCurrent : public SynapticInputEnvironment
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

} // namespace grenade::vx::network::abstract