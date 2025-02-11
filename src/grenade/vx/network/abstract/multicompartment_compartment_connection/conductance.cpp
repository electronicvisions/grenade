#include "grenade/vx/network/abstract/multicompartment_compartment_connection/conductance.h"

namespace grenade::vx::network::abstract {

// Constructor CompartmentConnectionConductance
CompartmentConnectionConductance::CompartmentConnectionConductance(double value)
{
	parameterization.conductance = value;
}

// Equal Operator CompartmentConnectionConductance
bool CompartmentConnectionConductance::is_equal_to(CompartmentConnection const& other) const
{
	CompartmentConnectionConductance const& other_cast =
	    static_cast<CompartmentConnectionConductance const&>(other);
	return (parameterization.conductance == other_cast.parameterization.conductance);
}

// Equal Operator CompartmentConnectionConductance Parameterization
bool CompartmentConnectionConductance::ParameterSpace::Parameterization::operator==(
    CompartmentConnectionConductance::ParameterSpace::Parameterization const& other) const
{
	return (conductance == other.conductance);
}

// Property Methods
std::unique_ptr<CompartmentConnection> CompartmentConnectionConductance::copy() const
{
	return std::make_unique<CompartmentConnectionConductance>(*this);
}
std::unique_ptr<CompartmentConnection> CompartmentConnectionConductance::move()
{
	return std::make_unique<CompartmentConnectionConductance>(std::move(*this));
}
std::ostream& CompartmentConnectionConductance::print(std::ostream& os) const
{
	return os << "CompartmentConnection_Capacitance" << parameterization.conductance;
}

// Checks if Parameterization is within Paramterspace
bool CompartmentConnectionConductance::ParameterSpace::contains(
    Parameterization const& parameterization)
{
	return (conductance_interval.contains(parameterization.conductance));
}

} // namespace grenade::vx::network::abstract