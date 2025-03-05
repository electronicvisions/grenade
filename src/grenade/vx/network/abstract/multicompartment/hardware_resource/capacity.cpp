#include "dapr/empty_property_impl.tcc"
#include "grenade/vx/network/abstract/multicompartment/hardware_resource/capacitance.h"

namespace dapr {
template struct EmptyProperty<
    grenade::vx::network::abstract::HardwareResourceCapacity,
    grenade::vx::network::abstract::HardwareResource>;

} // namespace dapr