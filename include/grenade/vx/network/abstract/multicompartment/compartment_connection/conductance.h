#pragma once

#include "ccalix/types.h"
#include "grenade/vx/network/abstract/multicompartment/compartment_connection.h"
#include "grenade/vx/network/abstract/parameter_interval.h"
#include <vector>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

// CompartmentConnectionConductance
struct SYMBOL_VISIBLE GENPYBIND(visible) CompartmentConnectionConductance
    : public CompartmentConnection
{
	CompartmentConnectionConductance() = default;

	struct ParameterSpace : public CompartmentConnection::ParameterSpace
	{
		std::vector<TimeInterval> time_constant;

		struct Parameterization : public CompartmentConnection::ParameterSpace::Parameterization
		{
			std::vector<ccalix::TimeInS> time_constant;

			Parameterization() = default;
			Parameterization(std::vector<ccalix::TimeInS> time_constant);

			virtual size_t size() const override;

			virtual std::unique_ptr<CompartmentConnection::ParameterSpace::Parameterization>
			get_section(grenade::common::MultiIndexSequence const& sequence) const override;

			virtual std::unique_ptr<CompartmentConnection::ParameterSpace::Parameterization> copy()
			    const override;
			virtual std::unique_ptr<CompartmentConnection::ParameterSpace::Parameterization> move()
			    override;

		protected:
			virtual std::ostream& print(std::ostream& os) const override;
			virtual bool is_equal_to(CompartmentConnection::ParameterSpace::Parameterization const&
			                             other) const override;
		};

		ParameterSpace() = default;
		ParameterSpace(std::vector<TimeInterval> time_constant);

		virtual size_t size() const override;

		virtual std::unique_ptr<CompartmentConnection::ParameterSpace> get_section(
		    grenade::common::MultiIndexSequence const& sequence) const override;

		virtual bool valid(CompartmentConnection::ParameterSpace::Parameterization const&
		                       parameterization) const override;

		virtual std::unique_ptr<CompartmentConnection::ParameterSpace> copy() const override;
		virtual std::unique_ptr<CompartmentConnection::ParameterSpace> move() override;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(CompartmentConnection::ParameterSpace const& other) const override;
	};

	// Graph Mechanisms
	std::unique_ptr<CompartmentConnection> copy() const;
	std::unique_ptr<CompartmentConnection> move();

protected:
	std::ostream& print(std::ostream& os) const;
	bool is_equal_to(CompartmentConnection const& other) const;
};

} // namespace abstract
} // namespace grenade::vx::network
