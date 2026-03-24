#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_current.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_input.h"
#include "grenade/vx/network/abstract/vertex_port_type/synaptic_input.h"
#include "hate/join.h"
#include <vector>

namespace grenade::vx::network::abstract {

MechanismSynapticInputCurrent::ParameterSpace::ParameterSpace(
    std::vector<ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>> i_synin_gm,
    std::vector<ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>> synapse_dac_bias,
    std::vector<ParameterInterval<ccalix::TimeInS>> time_constant) :
    i_synin_gm(std::move(i_synin_gm)),
    synapse_dac_bias(std::move(synapse_dac_bias)),
    time_constant(std::move(time_constant))
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
	ret.i_synin_gm.reserve(sequence.size());
	ret.synapse_dac_bias.reserve(sequence.size());
	ret.time_constant.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.i_synin_gm.push_back(i_synin_gm.at(element.value.at(0)));
		ret.synapse_dac_bias.push_back(synapse_dac_bias.at(element.value.at(0)));
		ret.time_constant.push_back(time_constant.at(element.value.at(0)));
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
	ret.i_synin_gm.reserve(sequence.size());
	ret.synapse_dac_bias.reserve(sequence.size());
	ret.time_constant.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.i_synin_gm.push_back(i_synin_gm.at(element.value.at(0)));
		ret.synapse_dac_bias.push_back(synapse_dac_bias.at(element.value.at(0)));
		ret.time_constant.push_back(time_constant.at(element.value.at(0)));
	}
	return std::make_unique<Parameterization>(std::move(ret));
}

size_t MechanismSynapticInputCurrent::ParameterSpace::size() const
{
	std::set<size_t> sizes;
	sizes.insert(i_synin_gm.size());
	sizes.insert(synapse_dac_bias.size());
	sizes.insert(time_constant.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameter space features heterogeneous size.");
	}
	return i_synin_gm.size();
}

MechanismSynapticInputCurrent::ParameterSpace::Parameterization::Parameterization(
    std::vector<lola::vx::v3::AtomicNeuron::AnalogValue> i_synin_gm,
    std::vector<lola::vx::v3::AtomicNeuron::AnalogValue> synapse_dac_bias,
    std::vector<ccalix::TimeInS> time_constant) :
    i_synin_gm(std::move(i_synin_gm)),
    synapse_dac_bias(std::move(synapse_dac_bias)),
    time_constant(std::move(time_constant))
{
}

size_t MechanismSynapticInputCurrent::ParameterSpace::Parameterization::size() const
{
	std::set<size_t> sizes;
	sizes.insert(i_synin_gm.size());
	sizes.insert(synapse_dac_bias.size());
	sizes.insert(time_constant.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameterization features heterogeneous size.");
	}
	return i_synin_gm.size();
}

bool MechanismSynapticInputCurrent::ParameterSpace::valid(
    Mechanism::ParameterSpace::Parameterization const& parameterization) const
{
	auto* cast_parameterization = dynamic_cast<Parameterization const*>(&parameterization);
	if (!cast_parameterization) {
		return false;
	}

	for (size_t i = 0; i < size(); ++i) {
		if (!i_synin_gm.at(i).contains(cast_parameterization->i_synin_gm.at(i))) {
			return false;
		}
		if (!synapse_dac_bias.at(i).contains(cast_parameterization->synapse_dac_bias.at(i))) {
			return false;
		}
		if (!time_constant.at(i).contains(cast_parameterization->time_constant.at(i))) {
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
	return (
	    i_synin_gm == other_cast->i_synin_gm && synapse_dac_bias == other_cast->synapse_dac_bias &&
	    time_constant == other_cast->time_constant);
}
std::ostream& MechanismSynapticInputCurrent::ParameterSpace::Parameterization::print(
    std::ostream& os) const
{
	os << "Parameterization(\n";
	os << "\tI_synin_gm: " << hate::join(i_synin_gm, ", ");
	os << "\n\tSynapse_dac_bias:" << hate::join(synapse_dac_bias, ", ");
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
	    i_synin_gm == other_cast->i_synin_gm && synapse_dac_bias == other_cast->synapse_dac_bias &&
	    time_constant == other_cast->time_constant);
}
std::ostream& MechanismSynapticInputCurrent::ParameterSpace::print(std::ostream& os) const
{
	os << "ParameterSpace(\n";
	os << "\tI_synin_gm: " << hate::join(i_synin_gm, ", ");
	os << "\n\tSynapse_dac_bias:" << hate::join(synapse_dac_bias, ", ");
	os << "\n\tTime-constant: " << hate::join(time_constant, ", ");
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