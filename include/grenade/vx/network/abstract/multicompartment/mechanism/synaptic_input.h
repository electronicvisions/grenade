#pragma once
#include "grenade/common/receptor_on_compartment.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/with_analog_readout.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism_environment.h"
#include "grenade/vx/network/receptor.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

// Mechanism for Synaptic Input
struct GENPYBIND(visible) SYMBOL_VISIBLE MechanismSynapticInput : public MechanismWithAnalogReadout
{
	// Check for conflict with itself when placed on compartment
	bool conflict(Mechanism const& other) const;

	// Constructor
	MechanismSynapticInput() = default;

	typedef Receptor::Type ReceptorType GENPYBIND(visible);

	MechanismSynapticInput(ReceptorType receptor_type, bool enable_analog_readout);

	ReceptorType receptor_type;

	// Return HardwareRessource requirements
	HardwareConstraints get_hardware(
	    Mechanism::ParameterSpace const& mechanism_parameter_space,
	    MechanismEnvironment const* environment) const;

	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const override;

	virtual lola::vx::v3::AtomicNeuron::Readout::Source get_analog_readout_source() const override;

protected:
	bool is_equal_to(Mechanism const& other) const override;
	std::ostream& print(std::ostream& os) const override;

	// Convert number of inputs to number of how much synaptic circuits are needed
	int round(int i) const;
};


} // namespace abstract
} // namespace grenade::vx::network
