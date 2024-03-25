#include "grenade/vx/network/abstract/multicompartment_synaptic_input_environment/conductance.h"

namespace grenade::vx::network {

SynapticInputEnvironmentConductance::SynapticInputEnvironmentConductance(
    bool exitatory_in, int number_in) :
    SynapticInputEnvironment(exitatory_in, number_in)
{}

SynapticInputEnvironmentConductance::SynapticInputEnvironmentConductance(
    bool exitatory_in, NumberTopBottom numbers) :
    SynapticInputEnvironment(exitatory_in, numbers)
{}

// Property Methods
std::unique_ptr<SynapticInputEnvironment> SynapticInputEnvironmentConductance::copy() const
{
	return std::make_unique<SynapticInputEnvironmentConductance>(*this);
}
std::unique_ptr<SynapticInputEnvironment> SynapticInputEnvironmentConductance::move()
{
	return std::make_unique<SynapticInputEnvironmentConductance>(std::move(*this));
}
std::ostream& SynapticInputEnvironmentConductance::print(std::ostream& os) const
{
	return os << "SynapticInputEnvironment_Conductance" << exitatory << number_of_inputs;
}
bool SynapticInputEnvironmentConductance::is_equal_to(
    SynapticInputEnvironmentConductance const& other) const
{
	return (number_of_inputs == other.number_of_inputs && exitatory == other.exitatory);
}

} // namespace grenade::vx::network