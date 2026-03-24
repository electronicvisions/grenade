#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_conductance.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_input.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/with_analog_readout.h"
#include "grenade/vx/network/abstract/vertex_port_type/synaptic_input.h"
#include "hate/join.h"

namespace grenade::vx::network::abstract {

MechanismSynapticInputConductance::ParameterSpace::ParameterSpace(
    std::vector<ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>> i_synin_gm,
    std::vector<ParameterInterval<lola::vx::v3::AtomicNeuron::AnalogValue>> synapse_dac_bias,
    std::vector<ParameterInterval<double>> e_reversal,
    std::vector<ParameterInterval<std::optional<double>>> e_reference,
    std::vector<ParameterInterval<ccalix::TimeInS>> time_constant) :
    i_synin_gm(std::move(i_synin_gm)),
    synapse_dac_bias(std::move(synapse_dac_bias)),
    e_reversal(std::move(e_reversal)),
    e_reference(std::move(e_reference)),
    time_constant(std::move(time_constant))
{
}

size_t MechanismSynapticInputConductance::ParameterSpace::size() const
{
	std::set<size_t> sizes;
	sizes.insert(i_synin_gm.size());
	sizes.insert(synapse_dac_bias.size());
	sizes.insert(e_reversal.size());
	sizes.insert(e_reference.size());
	sizes.insert(time_constant.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameter space features heterogeneous size.");
	}
	return i_synin_gm.size();
}

std::unique_ptr<Mechanism::ParameterSpace>
MechanismSynapticInputConductance::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	ParameterSpace ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in parameter space.");
	}
	ret.i_synin_gm.reserve(sequence.size());
	ret.synapse_dac_bias.reserve(sequence.size());
	ret.e_reversal.reserve(sequence.size());
	ret.e_reference.reserve(sequence.size());
	ret.time_constant.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.i_synin_gm.push_back(i_synin_gm.at(element.value.at(0)));
		ret.synapse_dac_bias.push_back(synapse_dac_bias.at(element.value.at(0)));
		ret.e_reversal.push_back(e_reversal.at(element.value.at(0)));
		ret.e_reference.push_back(e_reference.at(element.value.at(0)));
		ret.time_constant.push_back(time_constant.at(element.value.at(0)));
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
	ret.i_synin_gm.reserve(sequence.size());
	ret.synapse_dac_bias.reserve(sequence.size());
	ret.e_reversal.reserve(sequence.size());
	ret.e_reference.reserve(sequence.size());
	ret.time_constant.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		ret.i_synin_gm.push_back(i_synin_gm.at(element.value.at(0)));
		ret.synapse_dac_bias.push_back(synapse_dac_bias.at(element.value.at(0)));
		ret.e_reversal.push_back(e_reversal.at(element.value.at(0)));
		ret.e_reference.push_back(e_reference.at(element.value.at(0)));
		ret.time_constant.push_back(time_constant.at(element.value.at(0)));
	}
	return std::make_unique<Parameterization>(std::move(ret));
}

MechanismSynapticInputConductance::ParameterSpace::Parameterization::Parameterization(
    std::vector<lola::vx::v3::AtomicNeuron::AnalogValue> i_synin_gm,
    std::vector<lola::vx::v3::AtomicNeuron::AnalogValue> synapse_dac_bias,
    std::vector<double> e_reversal,
    std::vector<std::optional<double>> e_reference,
    std::vector<ccalix::TimeInS> time_constant) :
    i_synin_gm(std::move(i_synin_gm)),
    synapse_dac_bias(std::move(synapse_dac_bias)),
    e_reversal(std::move(e_reversal)),
    e_reference(std::move(e_reference)),
    time_constant(std::move(time_constant))
{
}

size_t MechanismSynapticInputConductance::ParameterSpace::Parameterization::size() const
{
	std::set<size_t> sizes;
	sizes.insert(i_synin_gm.size());
	sizes.insert(synapse_dac_bias.size());
	sizes.insert(e_reversal.size());
	sizes.insert(e_reference.size());
	sizes.insert(time_constant.size());
	if (sizes.size() != 1) {
		throw std::runtime_error("Parameterization features heterogeneous size.");
	}
	return i_synin_gm.size();
}

bool MechanismSynapticInputConductance::ParameterSpace::valid(
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
		if (!e_reversal.at(i).contains(cast_parameterization->e_reversal.at(i))) {
			return false;
		}
		if (!e_reference.at(i).contains(cast_parameterization->e_reference.at(i))) {
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
	    i_synin_gm == other_cast->i_synin_gm && synapse_dac_bias == other_cast->synapse_dac_bias &&
	    e_reversal == other_cast->e_reversal && e_reference == other_cast->e_reference &&
	    time_constant == other_cast->time_constant);
}
std::ostream& MechanismSynapticInputConductance::ParameterSpace::Parameterization::print(
    std::ostream& os) const
{
	os << "Parameterization(\n";
	os << "\tI_synin_gm: " << hate::join(i_synin_gm, ", ");
	os << "\n\tSynapse_dac_bias:" << hate::join(synapse_dac_bias, ", ");
	os << "\n\tE_reversal:" << hate::join(e_reversal, ", ");
	os << "\n\tE_reference:" << hate::join(e_reference, ", ", [](auto const& e) {
		std::stringstream ss;
		if (e) {
			ss << *e;
		} else {
			ss << "not specified";
		}
		return ss.str();
	});
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
	    i_synin_gm == other_cast->i_synin_gm && synapse_dac_bias == other_cast->synapse_dac_bias &&
	    e_reversal == other_cast->e_reversal && e_reference == other_cast->e_reference &&
	    time_constant == other_cast->time_constant);
}
std::ostream& MechanismSynapticInputConductance::ParameterSpace::print(std::ostream& os) const
{
	os << "ParameterSpace(\n";
	os << "\tI_synin_gm: " << hate::join(i_synin_gm, ", ");
	os << "\n\tSynapse_dac_bias:" << hate::join(synapse_dac_bias, ", ");
	os << "\n\tE_reversal:" << hate::join(e_reversal, ", ");
	os << "\n\tE_reference:" << hate::join(e_reference, ", ", [](auto const& e) {
		std::stringstream ss;
		ss << "[";
		if (e.get_lower()) {
			ss << *e.get_lower();
		} else {
			ss << "not specified";
		}
		ss << ", ";
		if (e.get_lower()) {
			ss << *e.get_lower();
		} else {
			ss << "disabled";
		}
		ss << "]";
		return ss.str();
	});
	os << "\n\tTime-constant: " << hate::join(time_constant, ", ");
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