#pragma once

#include "grenade/vx/network/abstract/multicompartment/mechanism.h"
#include "lola/vx/v3/neuron.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

// Mechanism with analog readout
struct GENPYBIND(visible) SYMBOL_VISIBLE MechanismWithAnalogReadout : public Mechanism
{
	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const override;
	virtual std::vector<grenade::common::Vertex::Port> get_output_ports() const override;

	MechanismWithAnalogReadout(bool enable_analog_readout = false);

	bool enable_analog_readout;

	virtual lola::vx::v3::AtomicNeuron::Readout::Source get_analog_readout_source() const = 0;

protected:
	virtual bool is_equal_to(Mechanism const& other) const override;
};


} // namespace abstract
} // namespace grenade::vx::network
