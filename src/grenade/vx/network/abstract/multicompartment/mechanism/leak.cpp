#include "grenade/vx/network/abstract/multicompartment/mechanism/leak.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "haldls/vx/v3/cadc.h"
#include "hate/join.h"
#include "hate/type_index.h"

namespace grenade::vx::network::abstract {

MechanismLeak::ParameterSpace::ParameterSpace(
    std::vector<ParameterInterval<ccalix::TimeInS>> time_constant,
    std::vector<ParameterInterval<haldls::vx::v3::CADCSampleQuad::Value>> potential) :
    time_constant(std::move(time_constant)), potential(std::move(potential))
{
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismLeak::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	ParameterSpace ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameter space.");
	}
	ret.time_constant.reserve(sequence.size());
	ret.potential.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.time_constant.push_back(time_constant.at(element.value.at(0)));
		ret.potential.push_back(potential.at(element.value.at(0)));
	}
	return std::make_unique<ParameterSpace>(std::move(ret));
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismLeak::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	Parameterization ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameterization.");
	}
	ret.time_constant.reserve(sequence.size());
	ret.potential.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.time_constant.push_back(time_constant.at(element.value.at(0)));
		ret.potential.push_back(potential.at(element.value.at(0)));
	}
	return std::make_unique<Parameterization>(std::move(ret));
}

size_t MechanismLeak::ParameterSpace::size() const
{
	std::set<size_t> sizes;
	sizes.insert(time_constant.size());
	sizes.insert(potential.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameter space features heterogeneous size.");
	}
	return time_constant.size();
}

MechanismLeak::ParameterSpace::Parameterization::Parameterization(
    std::vector<ccalix::TimeInS> time_constant,
    std::vector<haldls::vx::v3::CADCSampleQuad::Value> potential) :
    time_constant(std::move(time_constant)), potential(std::move(potential))
{
}

size_t MechanismLeak::ParameterSpace::Parameterization::size() const
{
	std::set<size_t> sizes;
	sizes.insert(time_constant.size());
	sizes.insert(potential.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameterization features heterogeneous size.");
	}
	return time_constant.size();
}

bool MechanismLeak::ParameterSpace::valid(
    Mechanism::ParameterSpace::Parameterization const& parameterization) const
{
	auto* cast_parameterization = dynamic_cast<Parameterization const*>(&parameterization);
	if (!cast_parameterization) {
		return false;
	}

	for (size_t i = 0; i < size(); ++i) {
		if (!time_constant.at(i).contains(cast_parameterization->time_constant.at(i))) {
			return false;
		}
		if (!potential.at(i).contains(cast_parameterization->potential.at(i))) {
			return false;
		}
	}
	return true;
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismLeak::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<MechanismLeak::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismLeak::ParameterSpace::Parameterization::move()
{
	return std::make_unique<MechanismLeak::ParameterSpace::Parameterization>(std::move(*this));
}

bool MechanismLeak::ParameterSpace::Parameterization::is_equal_to(
    Mechanism::ParameterSpace::Parameterization const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismLeak::ParameterSpace::Parameterization*>(&other);

	if (!other_cast) {
		return false;
	}
	return (time_constant == other_cast->time_constant) && (potential == other_cast->potential);
}

std::ostream& MechanismLeak::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	os << "Parameterization(\n"
	   << "\n\tLeak time_constant: " << hate::join(time_constant, ", ")
	   << "\n\t Leak Potential: " << hate::join(potential, ", ") << "\n)";
	return os;
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismLeak::ParameterSpace::copy() const
{
	return std::make_unique<MechanismLeak::ParameterSpace>(*this);
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismLeak::ParameterSpace::move()
{
	return std::make_unique<MechanismLeak::ParameterSpace>(std::move(*this));
}

bool MechanismLeak::ParameterSpace::is_equal_to(Mechanism::ParameterSpace const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismLeak::ParameterSpace*>(&other);

	if (!other_cast) {
		return false;
	}
	return (time_constant == other_cast->time_constant) && (potential == other_cast->potential);
}

std::ostream& MechanismLeak::ParameterSpace::print(std::ostream& os) const
{
	os << "Parameter-Space(\n"
	   << "\n\tLeak Time constant: " << hate::join(time_constant, ", ")
	   << "\n\tLeak Potential: " << hate::join(potential, ", ") << "\n)";
	return os;
}

bool MechanismLeak::conflict(Mechanism const& other) const
{
	return (typeid(*this) == typeid(other));
}

bool MechanismLeak::valid(Mechanism::ParameterSpace const&) const
{
	return true;
}

std::vector<grenade::common::Vertex::Port> MechanismLeak::get_input_ports() const
{
	return {};
}

std::vector<grenade::common::Vertex::Port> MechanismLeak::get_output_ports() const
{
	return {};
}

HardwareConstraints MechanismLeak::get_hardware(
    Mechanism::ParameterSpace const& mechanism_parameter_space, MechanismEnvironment const*) const
{
	const auto* parameter_space =
	    dynamic_cast<const MechanismLeak::ParameterSpace*>(&mechanism_parameter_space);

	if (!parameter_space) {
		throw("Given parameter space does not correspond to a leak-mechanism.");
	}

	HardwareConstraints constraints;

	// Leak mechanism requires a single neuron circuit. Usecases having a higher conductance with
	// multiple neuron circuits is currently not supported.
	HardwareConstraint constraint;
	constraint.resource = HardwareResourceLeak();
	constraint.numbers.number_total = 1;
	constraints.push_back(std::move(constraint));
	return constraints;
}

std::unique_ptr<Mechanism> MechanismLeak::copy() const
{
	return std::make_unique<MechanismLeak>(*this);
}

std::unique_ptr<Mechanism> MechanismLeak::move()
{
	return std::make_unique<MechanismLeak>(std::move(*this));
}

std::ostream& MechanismLeak::print(std::ostream& os) const
{
	os << "MechanismLeak";
	return os;
}

bool MechanismLeak::is_equal_to(Mechanism const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismLeak*>(&other);

	if (!other_cast) {
		return false;
	}
	return true;
}


} // namespace grenade::vx::network::abstract