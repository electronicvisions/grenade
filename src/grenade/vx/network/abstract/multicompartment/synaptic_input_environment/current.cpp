#include "grenade/vx/network/abstract/multicompartment/synaptic_input_environment/current.h"

namespace grenade::vx::network::abstract {

SynapticInputEnvironmentCurrent::SynapticInputEnvironmentCurrent(bool exitatory_in, int number_in) :
    SynapticInputEnvironment(exitatory_in, number_in)
{
}

SynapticInputEnvironmentCurrent::SynapticInputEnvironmentCurrent(
    bool exitatory_in, NumberTopBottom numbers) :
    SynapticInputEnvironment(exitatory_in, numbers)
{
}


// Property Methods
std::unique_ptr<SynapticInputEnvironment> SynapticInputEnvironmentCurrent::copy() const
{
	return std::make_unique<SynapticInputEnvironmentCurrent>(*this);
}
std::unique_ptr<SynapticInputEnvironment> SynapticInputEnvironmentCurrent::move()
{
	return std::make_unique<SynapticInputEnvironmentCurrent>(std::move(*this));
}
std::ostream& SynapticInputEnvironmentCurrent::print(std::ostream& os) const
{
	return os << "SynapticInputEnvironment_Current" << exitatory << number_of_inputs;
}
bool SynapticInputEnvironmentCurrent::is_equal_to(
    SynapticInputEnvironmentCurrent const& other) const
{
	return (number_of_inputs == other.number_of_inputs && exitatory == other.exitatory);
}

} // namepace grenade::vx::network