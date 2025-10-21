#pragma once

#include "grenade/vx/network/abstract/multicompartment_hardware_resource.h"
#include "grenade/vx/network/abstract/multicompartment_top_bottom.h"
#include <iostream>
#include <vector>

namespace grenade::vx::network::abstract {

struct SYMBOL_VISIBLE HardwareConstraint : dapr::Property<HardwareConstraint>
{
	NumberTopBottom numbers;
	dapr::PropertyHolder<HardwareResource> resource;

	// Property Methods
	std::unique_ptr<HardwareConstraint> copy() const;
	std::unique_ptr<HardwareConstraint> move();

protected:
	std::ostream& print(std::ostream& os) const;
	bool is_equal_to(HardwareConstraint const& other) const;
};

} // namespace grenade::vx::network::abstract