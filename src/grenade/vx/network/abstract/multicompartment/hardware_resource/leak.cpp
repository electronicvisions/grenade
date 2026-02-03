#include "grenade/vx/network/abstract/multicompartment/hardware_resource/leak.h"
#include "dapr/empty_property_impl.tcc"

namespace dapr {
template struct EmptyProperty<
    grenade::vx::network::abstract::HardwareResourceLeak,
    grenade::vx::network::abstract::HardwareResource>;

} // namespace dapr