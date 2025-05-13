#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "hate/visibility.h"

namespace grenade::vx::compute::detail {

execution::JITGraphExecutor::ChipConfigs get_chip_configs_from_chip(lola::vx::v3::Chip const& chip)
    SYMBOL_VISIBLE;

} // namespace grenade::vx::compute::detail
