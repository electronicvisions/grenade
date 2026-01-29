#include "lola/vx/v3/system.h"
#include <variant>

namespace grenade::vx::execution::detail {

using System =
    std::variant<lola::vx::v3::ChipAndMultichipJboaLeafFPGA, lola::vx::v3::ChipAndSinglechipFPGA>;

} // namespace grenade::vx::execution::detail
