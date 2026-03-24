#include "grenade/vx/network/abstract/multicompartment/mechanism/with_analog_readout.h"

#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/analog_readout.h"
#include "grenade/vx/network/abstract/vertex_port_type/analog_observable.h"

namespace grenade::vx::network::abstract {

MechanismWithAnalogReadout::MechanismWithAnalogReadout(bool enable_analog_readout) :
    enable_analog_readout(enable_analog_readout)
{
}

std::vector<grenade::common::Vertex::Port> MechanismWithAnalogReadout::get_input_ports() const
{
	return {};
}

std::vector<grenade::common::Vertex::Port> MechanismWithAnalogReadout::get_output_ports() const
{
	if (enable_analog_readout) {
		return {grenade::common::Vertex::Port(
		    AnalogObservable(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
		    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
		    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
		    grenade::common::ListMultiIndexSequence())};
	}
	return {};
}

bool MechanismWithAnalogReadout::is_equal_to(Mechanism const& other) const
{
	return enable_analog_readout ==
	       static_cast<MechanismWithAnalogReadout const&>(other).enable_analog_readout;
}

} // namespace grenade::vx::network::abstract