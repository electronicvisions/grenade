#include "grenade/vx/compute/detail/chip_configs.h"

namespace grenade::vx::compute::detail {

execution::JITGraphExecutor::ChipConfigs get_chip_configs_from_chip(lola::vx::v3::Chip const& chip)
{
	return {{grenade::common::ExecutionInstanceID(), {{common::ChipOnConnection(), chip}}}};
}

} // namespace grenade::vx::compute::detail
