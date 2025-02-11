#include "grenade/vx/network/abstract/multicompartment_hardware_resource/capacitance.h"

#include "grenade/common/empty_property_impl.tcc"

namespace grenade::common {

template struct EmptyProperty<
    vx::network::abstract::HardwareResourceCapacity,
    vx::network::abstract::HardwareResource>;

} // namespace::grenade::common