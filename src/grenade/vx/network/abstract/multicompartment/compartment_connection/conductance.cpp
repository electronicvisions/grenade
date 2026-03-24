#include "grenade/vx/network/abstract/multicompartment/compartment_connection/conductance.h"

#include "ccalix/types.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/network/abstract/multicompartment/compartment_connection.h"
#include "grenade/vx/network/abstract/parameter_interval.h"
#include "hate/join.h"

namespace grenade::vx::network::abstract {

CompartmentConnectionConductance::ParameterSpace::Parameterization::Parameterization(
    std::vector<ccalix::TimeInS> time_constant) :
    time_constant(std::move(time_constant))
{
}

size_t CompartmentConnectionConductance::ParameterSpace::Parameterization::size() const
{
	return time_constant.size();
}

std::unique_ptr<CompartmentConnection::ParameterSpace::Parameterization>
CompartmentConnectionConductance::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	Parameterization ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameterization.");
	}
	ret.time_constant.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.time_constant.push_back(time_constant.at(element.value.at(0)));
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
	return os << "Parameterization(" << hate::join(time_constant, ", ") << ")";
}

bool CompartmentConnectionConductance::ParameterSpace::Parameterization::is_equal_to(
    CompartmentConnection::ParameterSpace::Parameterization const& other) const
{
	return (time_constant == static_cast<Parameterization const&>(other).time_constant);
}

CompartmentConnectionConductance::ParameterSpace::ParameterSpace(
    std::vector<ParameterInterval<ccalix::TimeInS>> time_constant) :
    time_constant(std::move(time_constant))
{
}

size_t CompartmentConnectionConductance::ParameterSpace::size() const
{
	return time_constant.size();
}

std::unique_ptr<CompartmentConnection::ParameterSpace>
CompartmentConnectionConductance::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	ParameterSpace ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameter space.");
	}
	ret.time_constant.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.time_constant.push_back(time_constant.at(element.value.at(0)));
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
			if (!time_constant.at(i).contains(ptr->time_constant.at(i))) {
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
	return os << "ParameterSpace(" << hate::join(time_constant, ", ") << ")";
}

bool CompartmentConnectionConductance::ParameterSpace::is_equal_to(
    CompartmentConnection::ParameterSpace const& other) const
{
	return (time_constant == static_cast<ParameterSpace const&>(other).time_constant);
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