#pragma once
#include "grenade/vx/execution/jit_graph_executor.h"
#include "hate/visibility.h"


/**
 * Generate JITGraphExecutor::ChipConfigs with all neurons in excitatory bypass configuration.
 * The returned chip configs contain a single chip configuration for ExecutionInstance(DLSGlobal(0),
 * ExecutionIndex(0)).
 */
grenade::vx::execution::JITGraphExecutor::ChipConfigs get_chip_configs_bypass_excitatory()
    SYMBOL_VISIBLE;
