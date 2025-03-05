#pragma once

#include "grenade/vx/network/abstract/multicompartment/compartment_connection.h"

namespace grenade::vx::network::abstract {

// CompartmentConnectionConductance
struct SYMBOL_VISIBLE GENPYBIND(visible) CompartmentConnectionConductance
    : public CompartmentConnection
{
	CompartmentConnectionConductance() = default;
	CompartmentConnectionConductance(double value);
	struct ParameterSpace
	{
		ParameterInterval<double> conductance_interval;
		struct Parameterization
		{
			bool operator==(Parameterization const& other) const;
			double conductance;
		};
		bool contains(Parameterization const& conductance);
	};
	ParameterSpace parameter_space;
	ParameterSpace::Parameterization parameterization;

	// Graph Mechanisms
	std::unique_ptr<CompartmentConnection> copy() const;
	std::unique_ptr<CompartmentConnection> move();
	std::ostream& print(std::ostream& os) const;

protected:
	bool is_equal_to(CompartmentConnection const& other) const;
};

} // namespace grenade::vx::network::abstract