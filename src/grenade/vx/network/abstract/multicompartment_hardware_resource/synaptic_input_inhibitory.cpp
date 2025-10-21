#include "grenade/vx/network/abstract/multicompartment_hardware_resource/synaptic_input_inhibitory.h"

#include "dapr/empty_property_impl.tcc"

namespace dapr {

template struct EmptyProperty<
    grenade::vx::network::abstract::HardwareResourceSynapticInputInhibitory,
    grenade::vx::network::abstract::HardwareResource>;

} // namespace dapr