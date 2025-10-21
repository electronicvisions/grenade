#include "grenade/vx/network/abstract/multicompartment_hardware_resource/capacitance.h"

#include "dapr/empty_property_impl.tcc"

namespace dapr {

template struct EmptyProperty<
    grenade::vx::network::abstract::HardwareResourceCapacity,
    grenade::vx::network::abstract::HardwareResource>;

} // namespace dapr