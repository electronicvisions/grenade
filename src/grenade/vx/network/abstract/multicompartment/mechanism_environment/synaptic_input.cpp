#include "grenade/vx/network/abstract/multicompartment/mechanism_environment/synaptic_input.h"

namespace grenade::vx::network::abstract {

SynapticInputEnvironment::SynapticInputEnvironment(int number_in) :
    number_of_inputs(number_in, 0, 0)
{
}

SynapticInputEnvironment::SynapticInputEnvironment(NumberTopBottom numbers) :
    number_of_inputs(numbers)
{
}

// Property Methods
std::unique_ptr<MechanismEnvironment> SynapticInputEnvironment::copy() const
{
	return std::make_unique<SynapticInputEnvironment>(*this);
}
std::unique_ptr<MechanismEnvironment> SynapticInputEnvironment::move()
{
	return std::make_unique<SynapticInputEnvironment>(std::move(*this));
}
std::ostream& SynapticInputEnvironment::print(std::ostream& os) const
{
	return os << "SynapticInputEnvironment(" << number_of_inputs << ")";
}
bool SynapticInputEnvironment::is_equal_to(MechanismEnvironment const& other) const
{
	auto const& other_synaptic_input = static_cast<SynapticInputEnvironment const&>(other);
	return number_of_inputs == other_synaptic_input.number_of_inputs;
}

} // namespace grenade::vx::network::abstract