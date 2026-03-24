#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_conductance.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_input.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/with_analog_readout.h"
#include "grenade/vx/network/abstract/vertex_port_type/synaptic_input.h"
#include "hate/join.h"

namespace grenade::vx::network::abstract {

MechanismSynapticInputConductance::ParameterSpace::ParameterSpace(
    std::vector<ParameterInterval<double>> interval_conductance,
    std::vector<ParameterInterval<double>> interval_potential,
    std::vector<ParameterInterval<double>> interval_time_constant) :
    conductance_interval(std::move(interval_conductance)),
    potential_interval(std::move(interval_potential)),
    time_constant_interval(std::move(interval_time_constant))
{
}

size_t MechanismSynapticInputConductance::ParameterSpace::size() const
{
	std::set<size_t> sizes;
	sizes.insert(conductance_interval.size());
	sizes.insert(potential_interval.size());
	sizes.insert(time_constant_interval.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameter space features heterogeneous size.");
	}
	return conductance_interval.size();
}

std::unique_ptr<Mechanism::ParameterSpace>
MechanismSynapticInputConductance::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	ParameterSpace ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameter space.");
	}
	ret.conductance_interval.reserve(sequence.size());
	ret.potential_interval.reserve(sequence.size());
	ret.time_constant_interval.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.conductance_interval.push_back(conductance_interval.at(element.value.at(0)));
		ret.potential_interval.push_back(potential_interval.at(element.value.at(0)));
		ret.time_constant_interval.push_back(time_constant_interval.at(element.value.at(0)));
	}
	return std::make_unique<ParameterSpace>(std::move(ret));
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismSynapticInputConductance::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	Parameterization ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameterization.");
	}
	ret.conductance.reserve(sequence.size());
	ret.potential.reserve(sequence.size());
	ret.time_constant.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.conductance.push_back(conductance.at(element.value.at(0)));
		ret.potential.push_back(potential.at(element.value.at(0)));
		ret.time_constant.push_back(time_constant.at(element.value.at(0)));
	}
	return std::make_unique<Parameterization>(std::move(ret));
}

MechanismSynapticInputConductance::ParameterSpace::Parameterization::Parameterization(
    std::vector<double> conductance_in,
    std::vector<double> potential_in,
    std::vector<double> time_constant_in) :
    conductance(std::move(conductance_in)),
    potential(std::move(potential_in)),
    time_constant(std::move(time_constant_in))
{
}

size_t MechanismSynapticInputConductance::ParameterSpace::Parameterization::size() const
{
	std::set<size_t> sizes;
	sizes.insert(conductance.size());
	sizes.insert(potential.size());
	sizes.insert(time_constant.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameterization features heterogeneous size.");
	}
	return conductance.size();
}

bool MechanismSynapticInputConductance::ParameterSpace::valid(
    Mechanism::ParameterSpace::Parameterization const& parameterization) const
{
	auto* cast_parameterization = dynamic_cast<Parameterization const*>(&parameterization);
	if (!cast_parameterization) {
		return false;
	}

	for (size_t i = 0; i < size(); ++i) {
		if (!conductance_interval.at(i).contains(cast_parameterization->conductance.at(i))) {
			return false;
		}
		if (!potential_interval.at(i).contains(cast_parameterization->potential.at(i))) {
			return false;
		}
		if (!time_constant_interval.at(i).contains(cast_parameterization->time_constant.at(i))) {
			return false;
		}
	}
	return true;
}

// Property methods Parameterization
std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismSynapticInputConductance::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<MechanismSynapticInputConductance::ParameterSpace::Parameterization>(
	    *this);
}
std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismSynapticInputConductance::ParameterSpace::Parameterization::move()
{
	return std::make_unique<MechanismSynapticInputConductance::ParameterSpace::Parameterization>(
	    std::move(*this));
}
bool MechanismSynapticInputConductance::ParameterSpace::Parameterization::is_equal_to(
    Mechanism::ParameterSpace::Parameterization const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismSynapticInputConductance::ParameterSpace::Parameterization*>(
	        &other);

	if (!other_cast) {
		return false;
	}
	return (
	    conductance == other_cast->conductance && potential == other_cast->potential &&
	    time_constant == other_cast->time_constant);
}
std::ostream& MechanismSynapticInputConductance::ParameterSpace::Parameterization::print(
    std::ostream& os) const
{
	os << "Parameterization(\n";
	os << "\tConductance: " << hate::join(conductance, ", ");
	os << "\n\tPotential:" << hate::join(potential, ", ");
	os << "\n\tTime-constant: " << hate::join(time_constant, ", ");
	os << "\n)";
	return os;
}

// Property methods ParameterSpace
std::unique_ptr<Mechanism::ParameterSpace> MechanismSynapticInputConductance::ParameterSpace::copy()
    const
{
	return std::make_unique<MechanismSynapticInputConductance::ParameterSpace>(*this);
}
std::unique_ptr<Mechanism::ParameterSpace> MechanismSynapticInputConductance::ParameterSpace::move()
{
	return std::make_unique<MechanismSynapticInputConductance::ParameterSpace>(std::move(*this));
}
bool MechanismSynapticInputConductance::ParameterSpace::is_equal_to(
    Mechanism::ParameterSpace const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismSynapticInputConductance::ParameterSpace*>(&other);

	if (!other_cast) {
		return false;
	}
	return (
	    conductance_interval == other_cast->conductance_interval &&
	    potential_interval == other_cast->potential_interval &&
	    time_constant_interval == other_cast->time_constant_interval);
}
std::ostream& MechanismSynapticInputConductance::ParameterSpace::print(std::ostream& os) const
{
	os << "ParameterSpace(\n";
	os << "\tConductance: " << hate::join(conductance_interval, ", ");
	os << "\n\tPotential:" << hate::join(potential_interval, ", ");
	os << "\n\tTime-constant: " << hate::join(time_constant_interval, ", ");
	os << "\n)";
	return os;
}

MechanismSynapticInputConductance::MechanismSynapticInputConductance(
    ReceptorType receptor_type, bool enable_analog_readout) :
    MechanismSynapticInput(receptor_type, enable_analog_readout)
{
}

bool MechanismSynapticInputConductance::valid(Mechanism::ParameterSpace const&) const
{
	return true;
}

// Copy
std::unique_ptr<Mechanism> MechanismSynapticInputConductance::copy() const
{
	return std::make_unique<MechanismSynapticInputConductance>(*this);
}
// Move
std::unique_ptr<Mechanism> MechanismSynapticInputConductance::move()
{
	return std::make_unique<MechanismSynapticInputConductance>(std::move(*this));
}

// Print
std::ostream& MechanismSynapticInputConductance::print(std::ostream& os) const
{
	os << "MechanismSynapticInputConductance(";
	MechanismSynapticInput::print(os) << ")";
	return os;
}

// Equality-Operator and Inequality-Operator
bool MechanismSynapticInputConductance::is_equal_to(Mechanism const& other) const
{
	return MechanismSynapticInput::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract