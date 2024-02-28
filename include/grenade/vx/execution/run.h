#pragma once
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/signal_flow/output_data.h"
#include "hate/visibility.h"
#include <vector>


namespace grenade::vx {

namespace signal_flow {
struct InputData;
class Graph;
} // namespace signal_flow

} // namespace grenade::vx

namespace grenade::vx::execution {

/**
 * Run the specified graph with specified inputs on the supplied executor.
 * @param executor Executor to use
 * @param graph Graph to execute
 * @param initial_config Map of initial configuration
 * @param input List of input values to use
 * @param hooks Map of playback sequence collections to be inserted at specified
 * execution instances
 */
signal_flow::OutputData run(
    JITGraphExecutor& executor,
    signal_flow::Graph const& graph,
    JITGraphExecutor::ChipConfigs const& initial_config,
    signal_flow::InputData const& input,
    JITGraphExecutor::Hooks&& hooks = {}) SYMBOL_VISIBLE;

/**
 * Run the specified graphs with specified inputs on the supplied executor.
 * @param executor Executor to use
 * @param graphs Graphs to execute (one per realtime_column)
 * @param configs Maps of configurations (one per realtime_column)
 * @param input Lists of input values to use (one per realtime_column)
 * @param hooks Map of playback sequence collections to be inserted at specified
 * execution instances
 */
std::vector<signal_flow::OutputData> run(
    JITGraphExecutor& executor,
    std::vector<std::reference_wrapper<signal_flow::Graph const>> const& graphs,
    std::vector<std::reference_wrapper<JITGraphExecutor::ChipConfigs const>> const& configs,
    std::vector<std::reference_wrapper<signal_flow::InputData const>> const& inputs,
    JITGraphExecutor::Hooks&& hooks = {}) SYMBOL_VISIBLE;

} // namespace grenade::vx::execution
