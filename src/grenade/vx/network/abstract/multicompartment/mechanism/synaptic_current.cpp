#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_current.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_input.h"
#include "grenade/vx/network/abstract/vertex_port_type/synaptic_input.h"
#include "hate/join.h"
#include <vector>

namespace grenade::vx::network::abstract {

MechanismSynapticInputCurrent::ParameterSpace::ParameterSpace(
    std::vector<ParameterInterval<double>> interval_current_in,
    std::vector<ParameterInterval<double>> interval_time_constant_in) :
    current_interval(std::move(interval_current_in)),
    time_constant_interval(std::move(interval_time_constant_in))
{
}

std::unique_ptr<Mechanism::ParameterSpace>
MechanismSynapticInputCurrent::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	ParameterSpace ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameter space.");
	}
	ret.current_interval.reserve(sequence.size());
	ret.time_constant_interval.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.current_interval.push_back(current_interval.at(element.value.at(0)));
		ret.time_constant_interval.push_back(time_constant_interval.at(element.value.at(0)));
	}
	return std::make_unique<ParameterSpace>(std::move(ret));
}

std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismSynapticInputCurrent::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	Parameterization ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameterization.");
	}
	ret.current.reserve(sequence.size());
	ret.time_constant.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.current.push_back(current.at(element.value.at(0)));
		ret.time_constant.push_back(time_constant.at(element.value.at(0)));
	}
	return std::make_unique<Parameterization>(std::move(ret));
}

size_t MechanismSynapticInputCurrent::ParameterSpace::size() const
{
	std::set<size_t> sizes;
	sizes.insert(current_interval.size());
	sizes.insert(time_constant_interval.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameter space features heterogeneous size.");
	}
	return current_interval.size();
}

MechanismSynapticInputCurrent::ParameterSpace::Parameterization::Parameterization(
    std::vector<double> current_in, std::vector<double> time_constant_in) :
    current(std::move(current_in)), time_constant(std::move(time_constant_in))
{
}

size_t MechanismSynapticInputCurrent::ParameterSpace::Parameterization::size() const
{
	std::set<size_t> sizes;
	sizes.insert(current.size());
	sizes.insert(time_constant.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameterization features heterogeneous size.");
	}
	return current.size();
}

bool MechanismSynapticInputCurrent::ParameterSpace::valid(
    Mechanism::ParameterSpace::Parameterization const& parameterization) const
{
	auto* cast_parameterization = dynamic_cast<Parameterization const*>(&parameterization);
	if (!cast_parameterization) {
		return false;
	}

	for (size_t i = 0; i < size(); ++i) {
		if (!current_interval.at(i).contains(cast_parameterization->current.at(i))) {
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
MechanismSynapticInputCurrent::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<MechanismSynapticInputCurrent::ParameterSpace::Parameterization>(*this);
}
std::unique_ptr<Mechanism::ParameterSpace::Parameterization>
MechanismSynapticInputCurrent::ParameterSpace::Parameterization::move()
{
	return std::make_unique<MechanismSynapticInputCurrent::ParameterSpace::Parameterization>(
	    std::move(*this));
}
bool MechanismSynapticInputCurrent::ParameterSpace::Parameterization::is_equal_to(
    Mechanism::ParameterSpace::Parameterization const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismSynapticInputCurrent::ParameterSpace::Parameterization*>(
	        &other);

	if (!other_cast) {
		return false;
	}
	return (current == other_cast->current && time_constant == other_cast->time_constant);
}
std::ostream& MechanismSynapticInputCurrent::ParameterSpace::Parameterization::print(
    std::ostream& os) const
{
	os << "Parameterization(\n";
	os << "\tCurrent: " << hate::join(current, ", ");
	os << "\n\tTime-constant: " << hate::join(time_constant, ", ");
	os << "\n)";
	return os;
}

// Property methods ParameterSpace
std::unique_ptr<Mechanism::ParameterSpace> MechanismSynapticInputCurrent::ParameterSpace::copy()
    const
{
	return std::make_unique<MechanismSynapticInputCurrent::ParameterSpace>(*this);
}
std::unique_ptr<Mechanism::ParameterSpace> MechanismSynapticInputCurrent::ParameterSpace::move()
{
	return std::make_unique<MechanismSynapticInputCurrent::ParameterSpace>(std::move(*this));
}
bool MechanismSynapticInputCurrent::ParameterSpace::is_equal_to(
    Mechanism::ParameterSpace const& other) const
{
	const auto* other_cast =
	    dynamic_cast<const MechanismSynapticInputCurrent::ParameterSpace*>(&other);

	if (!other_cast) {
		return false;
	}
	return (
	    current_interval == other_cast->current_interval &&
	    time_constant_interval == other_cast->time_constant_interval);
}
std::ostream& MechanismSynapticInputCurrent::ParameterSpace::print(std::ostream& os) const
{
	os << "ParameterSpace(\n";
	os << "\tCurrent: " << hate::join(current_interval, ", ");
	os << "\n\tTime-constant: " << hate::join(time_constant_interval, ", ");
	os << "\n)";
	return os;
}


MechanismSynapticInputCurrent::MechanismSynapticInputCurrent(
    ReceptorType receptor_type, bool enable_analog_readout) :
    MechanismSynapticInput(receptor_type, enable_analog_readout)
{
}

bool MechanismSynapticInputCurrent::valid(Mechanism::ParameterSpace const&) const
{
	return true;
}

// Copy
std::unique_ptr<Mechanism> MechanismSynapticInputCurrent::copy() const
{
	return std::make_unique<MechanismSynapticInputCurrent>(*this);
}
// Move
std::unique_ptr<Mechanism> MechanismSynapticInputCurrent::move()
{
	return std::make_unique<MechanismSynapticInputCurrent>(std::move(*this));
}
// Print
std::ostream& MechanismSynapticInputCurrent::print(std::ostream& os) const
{
	os << "MechanismSynapticInputCurrent(";
	MechanismSynapticInput::print(os) << ")";
	return os;
}

bool MechanismSynapticInputCurrent::is_equal_to(Mechanism const& other) const
{
	return MechanismSynapticInput::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract