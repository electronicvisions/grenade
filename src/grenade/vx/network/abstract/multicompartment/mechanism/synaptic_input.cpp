#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_input.h"

#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/analog_readout.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_excitatory.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/synaptic_input_inhibitory.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_environment/synaptic_input.h"
#include "grenade/vx/network/abstract/vertex_port_type/synaptic_input.h"
#include "hate/join.h"

namespace grenade::vx::network::abstract {

MechanismSynapticInput::MechanismSynapticInput(
    ReceptorType receptor_type, bool enable_analog_readout) :
    MechanismWithAnalogReadout(enable_analog_readout), receptor_type(receptor_type)
{
}

lola::vx::v3::AtomicNeuron::Readout::Source MechanismSynapticInput::get_analog_readout_source()
    const
{
	return receptor_type == ReceptorType::excitatory
	           ? lola::vx::v3::AtomicNeuron::Readout::Source::exc_synin
	           : lola::vx::v3::AtomicNeuron::Readout::Source::inh_synin;
}

bool MechanismSynapticInput::conflict(Mechanism const&) const
{
	return false;
}

std::vector<grenade::common::Vertex::Port> MechanismSynapticInput::get_input_ports() const
{
	return {grenade::common::Vertex::Port(
	    SynapticInput(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::ListMultiIndexSequence())};
}

int MechanismSynapticInput::round(int i) const
{
	if (i % halco::hicann_dls::vx::SynapseRowOnSynram::size == 0) {
		return (i / halco::hicann_dls::vx::SynapseRowOnSynram::size);
	} else {
		return ((i / halco::hicann_dls::vx::SynapseRowOnSynram::size) + 1);
	}
}

// Return HardwareRessource requirements
HardwareConstraints MechanismSynapticInput::get_hardware(
    Mechanism::ParameterSpace const&, MechanismEnvironment const* environment) const
{
	// Return Object and Input
	HardwareConstraints constraints;

	if (!environment) {
		throw std::invalid_argument("SynapticInput mechanism requires environment.");
	}
	auto const synaptic_input_environment =
	    dynamic_cast<SynapticInputEnvironment const*>(environment);

	if (!synaptic_input_environment) {
		throw std::invalid_argument("Environment invalid for mechanism.");
	}

	// Calculate Number of Synaptic Circuits required
	HardwareConstraint constraint;

	constraint.numbers.number_total =
	    round(synaptic_input_environment->number_of_inputs.number_total);
	constraint.numbers.number_top = round(synaptic_input_environment->number_of_inputs.number_top);
	constraint.numbers.number_bottom =
	    round(synaptic_input_environment->number_of_inputs.number_bottom);

	// Always request one neuron circuit instead of none
	if (constraint.numbers.number_total == 0) {
		constraint.numbers.number_total = 1;
	}

	if (receptor_type == ReceptorType::excitatory) {
		constraint.resource = HardwareResourceSynapticInputExitatory();
	} else {
		constraint.resource = HardwareResourceSynapticInputInhibitory();
	}

	constraints.push_back(constraint);

	if (enable_analog_readout) {
		HardwareConstraint readout_constraint;
		readout_constraint.resource = HardwareResourceAnalogReadout();
		if (constraint.numbers.number_top == constraint.numbers.number_total) {
			readout_constraint.numbers.number_top = 1;
		} else if (constraint.numbers.number_bottom == constraint.numbers.number_total) {
			readout_constraint.numbers.number_bottom = 1;
		}
		readout_constraint.numbers.number_total = 1;
		constraints.push_back(readout_constraint);
	}

	return constraints;
}

std::ostream& MechanismSynapticInput::print(std::ostream& os) const
{
	os << receptor_type << ", enable_analog_readout: " << enable_analog_readout;
	return os;
}

bool MechanismSynapticInput::is_equal_to(Mechanism const& other) const
{
	auto const* other_cast = static_cast<const MechanismSynapticInput*>(&other);
	if (!other_cast) {
		return false;
	}
	return receptor_type == other_cast->receptor_type &&
	       MechanismWithAnalogReadout::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract