#include "grenade/vx/network/abstract/multicompartment/mechanism/fire.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "haldls/vx/v3/cadc.h"
#include "hate/join.h"
#include "hate/type_index.h"

namespace grenade::vx::network::abstract {

MechanismFire::ParameterSpace::ParameterSpace(
    std::vector<ParameterInterval<haldls::vx::v3::CADCSampleQuad::Value>> threshold_potential,
    std::vector<ParameterInterval<haldls::vx::v3::CADCSampleQuad::Value>> reset_potential,
    std::vector<ParameterInterval<ccalix::TimeInS>> refractory_time,
    std::vector<ParameterInterval<ccalix::TimeInS>> holdoff_time) :
    threshold_potential(std::move(threshold_potential)),
    reset_potential(std::move(reset_potential)),
    refractory_time(std::move(refractory_time)),
    holdoff_time(std::move(holdoff_time))
{
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismFire::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	ParameterSpace ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameterization.");
	}
	ret.threshold_potential.reserve(sequence.size());
	ret.reset_potential.reserve(sequence.size());
	ret.refractory_time.reserve(sequence.size());
	ret.holdoff_time.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.threshold_potential.push_back(threshold_potential.at(element.value.at(0)));
		ret.reset_potential.push_back(reset_potential.at(element.value.at(0)));
		ret.refractory_time.push_back(refractory_time.at(element.value.at(0)));
		ret.holdoff_time.push_back(holdoff_time.at(element.value.at(0)));
	}
	return std::make_unique<ParameterSpace>(std::move(ret));
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismFire::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	Parameterization ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameter space.");
	}
	ret.threshold_potential.reserve(sequence.size());
	ret.reset_potential.reserve(sequence.size());
	ret.refractory_time.reserve(sequence.size());
	ret.holdoff_time.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.threshold_potential.push_back(threshold_potential.at(element.value.at(0)));
		ret.reset_potential.push_back(reset_potential.at(element.value.at(0)));
		ret.refractory_time.push_back(refractory_time.at(element.value.at(0)));
		ret.holdoff_time.push_back(holdoff_time.at(element.value.at(0)));
	}
	return std::make_unique<Parameterization>(std::move(ret));
}

size_t MechanismFire::ParameterSpace::size() const
{
	std::set<size_t> sizes;
	sizes.insert(threshold_potential.size());
	sizes.insert(reset_potential.size());
	sizes.insert(refractory_time.size());
	sizes.insert(holdoff_time.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameter space features heterogeneous size.");
	}
	return refractory_time.size();
}

MechanismFire::ParameterSpace::Parameterization::Parameterization(
    std::vector<haldls::vx::v3::CADCSampleQuad::Value> threshold_potential,
    std::vector<haldls::vx::v3::CADCSampleQuad::Value> reset_potential,
    std::vector<ccalix::TimeInS> refractory_time,
    std::vector<ccalix::TimeInS> holdoff_time) :
    threshold_potential(std::move(threshold_potential)),
    reset_potential(std::move(reset_potential)),
    refractory_time(std::move(refractory_time)),
    holdoff_time(std::move(holdoff_time))
{
}

size_t MechanismFire::ParameterSpace::Parameterization::size() const
{
	std::set<size_t> sizes;
	sizes.insert(threshold_potential.size());
	sizes.insert(reset_potential.size());
	sizes.insert(refractory_time.size());
	sizes.insert(holdoff_time.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameterization features heterogeneous size.");
	}
	return refractory_time.size();
}

bool MechanismFire::ParameterSpace::valid(
    Mechanism::ParameterSpace::Parameterization const& parameterization) const
{
	auto* cast_parameterization = dynamic_cast<Parameterization const*>(&parameterization);
	if (!cast_parameterization) {
		return false;
	}

	for (size_t i = 0; i < size(); ++i) {
		if (!threshold_potential.at(i).contains(cast_parameterization->threshold_potential.at(i))) {
			return false;
		}
		if (!reset_potential.at(i).contains(cast_parameterization->reset_potential.at(i))) {
			return false;
		}
		if (!refractory_time.at(i).contains(cast_parameterization->refractory_time.at(i))) {
			return false;
		}
		if (!holdoff_time.at(i).contains(cast_parameterization->holdoff_time.at(i))) {
			return false;
		}
	}
	return true;
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismFire::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<MechanismFire::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismFire::ParameterSpace::Parameterization::move()
{
	return std::make_unique<MechanismFire::ParameterSpace::Parameterization>(std::move(*this));
}

bool MechanismFire::ParameterSpace::Parameterization::is_equal_to(
    Mechanism::ParameterSpace::Parameterization const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismFire::ParameterSpace::Parameterization*>(&other);

	if (!other_cast) {
		return false;
	}
	return threshold_potential == other_cast->threshold_potential &&
	       reset_potential == other_cast->reset_potential &&
	       refractory_time == other_cast->refractory_time &&
	       holdoff_time == other_cast->holdoff_time;
}

std::ostream& MechanismFire::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	os << "Parameterization(\n"
	   << "\n\tThreshold Potential: " << hate::join(threshold_potential, ", ")
	   << "\n\tReset Potential: " << hate::join(reset_potential, ", ")
	   << "\n\tRefractory Time: " << hate::join(refractory_time, ", ")
	   << "\n\tHoldoff Time: " << hate::join(holdoff_time, ", ") << "\n)";
	return os;
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismFire::ParameterSpace::copy() const
{
	return std::make_unique<MechanismFire::ParameterSpace>(*this);
}

std::unique_ptr<Mechanism::ParameterSpace> MechanismFire::ParameterSpace::move()
{
	return std::make_unique<MechanismFire::ParameterSpace>(std::move(*this));
}

bool MechanismFire::ParameterSpace::is_equal_to(Mechanism::ParameterSpace const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismFire::ParameterSpace*>(&other);

	if (!other_cast) {
		return false;
	}
	return threshold_potential == other_cast->threshold_potential &&
	       reset_potential == other_cast->reset_potential &&
	       refractory_time == other_cast->refractory_time &&
	       holdoff_time == other_cast->holdoff_time;
}

std::ostream& MechanismFire::ParameterSpace::print(std::ostream& os) const
{
	os << "Parameter-Space(\n"
	   << "\n\tThreshold Potential: " << hate::join(threshold_potential, ", ")
	   << "\n\tReset Potential: " << hate::join(reset_potential, ", ")
	   << "\n\tRefractory Time: " << hate::join(refractory_time, ", ")
	   << "\n\tHoldoff Time: " << hate::join(holdoff_time, ", ") << "\n)";
	return os;
}

bool MechanismFire::conflict(Mechanism const& other) const
{
	return (typeid(*this) == typeid(other));
}

bool MechanismFire::valid(Mechanism::ParameterSpace const&) const
{
	return true;
}

std::vector<grenade::common::Vertex::Port> MechanismFire::get_input_ports() const
{
	return {};
}

std::vector<grenade::common::Vertex::Port> MechanismFire::get_output_ports() const
{
	return {};
}

HardwareConstraints MechanismFire::get_hardware(
    Mechanism::ParameterSpace const& mechanism_parameter_space, MechanismEnvironment const*) const
{
	const auto* parameter_space =
	    dynamic_cast<const MechanismFire::ParameterSpace*>(&mechanism_parameter_space);

	if (!parameter_space) {
		throw("Given parameter space does not correspond to a fire-mechanism.");
	}

	HardwareConstraints constraints;

	// Fire mechanism requires a single neuron circuit.
	HardwareConstraint constraint;
	constraint.resource = HardwareResourceFire();
	constraint.numbers.number_total = 1;
	constraints.push_back(std::move(constraint));
	return constraints;
}

std::unique_ptr<Mechanism> MechanismFire::copy() const
{
	return std::make_unique<MechanismFire>(*this);
}

std::unique_ptr<Mechanism> MechanismFire::move()
{
	return std::make_unique<MechanismFire>(std::move(*this));
}

std::ostream& MechanismFire::print(std::ostream& os) const
{
	os << "MechanismFire";
	return os;
}

bool MechanismFire::is_equal_to(Mechanism const& other) const
{
	const auto* other_cast = dynamic_cast<const MechanismFire*>(&other);

	if (!other_cast) {
		return false;
	}
	return true;
}


} // namespace grenade::vx::network::abstract