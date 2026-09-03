#pragma once
#include "grenade/vx/execution/jit_graph_executor.h"
#include "hate/visibility.h"


/**
 * Check if connections contained in the executor are all jboa connections.
 * @param executor Executor to check for connection type.
 * @param size Number of single connctions that are required on each multiconnection. No size
 * specification if emtpy.
 */
bool is_jboa_setup_of_size(
    grenade::vx::execution::JITGraphExecutor const& executor, size_t size = 0) SYMBOL_VISIBLE;
