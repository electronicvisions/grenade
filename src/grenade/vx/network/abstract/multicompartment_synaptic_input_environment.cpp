#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment.h"

namespace grenade::vx::network {

SynapticInputEnvironment::SynapticInputEnvironment(bool exitatory_in, int number_in) :
    exitatory(exitatory_in), number_of_inputs(number_in, 0, 0)
{}

SynapticInputEnvironment::SynapticInputEnvironment(bool exitatory, NumberTopBottom numbers) :
    exitatory(exitatory), number_of_inputs(numbers)
{}

// Property Methods
std::unique_ptr<SynapticInputEnvironment> SynapticInputEnvironment::copy() const
{
	return std::make_unique<SynapticInputEnvironment>(*this);
}
std::unique_ptr<SynapticInputEnvironment> SynapticInputEnvironment::move()
{
	return std::make_unique<SynapticInputEnvironment>(std::move(*this));
}
std::ostream& SynapticInputEnvironment::print(std::ostream& os) const
{
	return os << "SynapticInputEnvironment" << exitatory << number_of_inputs;
}
bool SynapticInputEnvironment::is_equal_to(SynapticInputEnvironment const& other) const
{
	return (number_of_inputs == other.number_of_inputs && exitatory == other.exitatory);
}

} // namespace grenade::vx::network