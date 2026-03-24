#include "grenade/vx/network/abstract/multicompartment/compartment_connection/conductance.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/network/abstract/multicompartment/compartment_connection.h"
#include "hate/join.h"

namespace grenade::vx::network::abstract {

size_t CompartmentConnectionConductance::ParameterSpace::Parameterization::size() const
{
	return conductance.size();
}

std::unique_ptr<CompartmentConnection::ParameterSpace::Parameterization>
CompartmentConnectionConductance::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	Parameterization ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameterization.");
	}
	ret.conductance.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.conductance.push_back(conductance.at(element.value.at(0)));
	}
	return std::make_unique<ParameterSpace::Parameterization>(std::move(ret));
}

std::unique_ptr<CompartmentConnection::ParameterSpace::Parameterization>
CompartmentConnectionConductance::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<CompartmentConnection::ParameterSpace::Parameterization>
CompartmentConnectionConductance::ParameterSpace::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

std::ostream& CompartmentConnectionConductance::ParameterSpace::Parameterization::print(
    std::ostream& os) const
{
	return os << "Parameterization(" << hate::join(conductance, ", ") << ")";
}

bool CompartmentConnectionConductance::ParameterSpace::Parameterization::is_equal_to(
    CompartmentConnection::ParameterSpace::Parameterization const& other) const
{
	return (conductance == static_cast<Parameterization const&>(other).conductance);
}

size_t CompartmentConnectionConductance::ParameterSpace::size() const
{
	return conductance_interval.size();
}

std::unique_ptr<CompartmentConnection::ParameterSpace>
CompartmentConnectionConductance::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	ParameterSpace ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameter space.");
	}
	ret.conductance_interval.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.conductance_interval.push_back(conductance_interval.at(element.value.at(0)));
	}
	return std::make_unique<ParameterSpace>(std::move(ret));
}

// Checks if Parameterization lies within ParameterSpace
bool CompartmentConnectionConductance::ParameterSpace::valid(
    CompartmentConnection::ParameterSpace::Parameterization const& parameterization) const
{
	if (auto const ptr = dynamic_cast<Parameterization const*>(&parameterization); ptr) {
		if (size() != ptr->size()) {
			return false;
		}
		for (size_t i = 0; i < size(); ++i) {
			if (!conductance_interval.at(i).contains(ptr->conductance.at(i))) {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::unique_ptr<CompartmentConnection::ParameterSpace>
CompartmentConnectionConductance::ParameterSpace::copy() const
{
	return std::make_unique<ParameterSpace>(*this);
}

std::unique_ptr<CompartmentConnection::ParameterSpace>
CompartmentConnectionConductance::ParameterSpace::move()
{
	return std::make_unique<ParameterSpace>(std::move(*this));
}

std::ostream& CompartmentConnectionConductance::ParameterSpace::print(std::ostream& os) const
{
	return os << "ParameterSpace(" << hate::join(conductance_interval, ", ") << ")";
}

bool CompartmentConnectionConductance::ParameterSpace::is_equal_to(
    CompartmentConnection::ParameterSpace const& other) const
{
	return (conductance_interval == static_cast<ParameterSpace const&>(other).conductance_interval);
}

// Equal Operator CompartmentConnectionConductance
bool CompartmentConnectionConductance::is_equal_to(CompartmentConnection const& /* other */) const
{
	return true;
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
	return os << "CompartmentConnectionConductance()";
}

} // namespace grenade::vx::network::abstract