#include "grenade/vx/network/abstract/multicompartment/hardware_constraint.h"

namespace grenade::vx::network::abstract {

// Property Methods
std::unique_ptr<HardwareConstraint> HardwareConstraint::copy() const
{
	return std::make_unique<HardwareConstraint>(*this);
}
std::unique_ptr<HardwareConstraint> HardwareConstraint::move()
{
	return std::make_unique<HardwareConstraint>(std::move(*this));
}
std::ostream& HardwareConstraint::print(std::ostream& os) const
{
	return os << "HardwareConstraint: " << numbers;
}
bool HardwareConstraint::is_equal_to(HardwareConstraint const& other) const
{
	return (numbers == other.numbers && resource == other.resource);
}


} // namespace grenade::vx::network::abstract