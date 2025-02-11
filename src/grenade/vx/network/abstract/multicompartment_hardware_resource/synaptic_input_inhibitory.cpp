#include "grenade/vx/network/abstract/multicompartment_hardware_resource/synaptic_input_inhibitory.h"

#include "grenade/common/empty_property_impl.tcc"

namespace grenade::common {

template struct EmptyProperty<
    vx::network::abstract::HardwareResourceSynapticInputInhibitory,
    vx::network::abstract::HardwareResource>;

} // namespace::grenade::common