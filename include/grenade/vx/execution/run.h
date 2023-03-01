#pragma once
#include "grenade/vx/execution/jit_graph_executor.h"
#include "hate/visibility.h"


namespace grenade::vx {

namespace signal_flow {
struct IODataMap;
class IODataList;
} // namespace signal_flow

namespace signal_flow {
class Graph;
} // namespace signal_flow

} // namespace grenade::vx

namespace grenade::vx::execution GENPYBIND_TAG_GRENADE_VX_EXECUTION {

/**
 * Run the specified graph with specified inputs on the supplied executor.
 * @param executor Executor to use
 * @param graph Graph to execute
 * @param input List of input values to use
 * @param initial_config Map of initial configuration
 * @param playback_hooks Map of playback sequence collections to be inserted at specified
 * execution instances
 */
signal_flow::IODataMap run(
    JITGraphExecutor& executor,
    signal_flow::Graph const& graph,
    signal_flow::IODataMap const& input,
    JITGraphExecutor::ChipConfigs const& initial_config,
    JITGraphExecutor::PlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

/**
 * Run the specified graph with specified inputs on the supplied executor.
 * @param executor Executor to use
 * @param graph Graph to execute
 * @param input List of input values to use
 * @param initial_config Map of initial configuration
 */
signal_flow::IODataMap run(
    JITGraphExecutor& executor,
    signal_flow::Graph const& graph,
    signal_flow::IODataMap const& input,
    JITGraphExecutor::ChipConfigs const& initial_config) SYMBOL_VISIBLE;

/**
 * Run the specified graph with specified inputs on the supplied executor.
 * @param executor Executor to use
 * @param graph Graph to execute
 * @param input List of input values to use
 * @param initial_config Map of initial configuration
 * @param playback_hooks List of playback sequence collections to be inserted at specified
 * execution instances
 * @param only_unconnected_output Whether to return only values to output vertices without out
 * edges
 */
signal_flow::IODataList run(
    JITGraphExecutor& executor,
    signal_flow::Graph const& graph,
    signal_flow::IODataList const& input,
    JITGraphExecutor::ChipConfigs const& initial_config,
    JITGraphExecutor::PlaybackHooks& playback_hooks,
    bool only_unconnected_output = true) SYMBOL_VISIBLE;

/**
 * Run the specified graph with specified inputs on the supplied executor.
 * @param executor Executor to use
 * @param graph Graph to execute
 * @param input List of input values to use
 * @param initial_config Map of initial configuration
 * @param only_unconnected_output Whether to return only values to output vertices without out
 * edges
 */
signal_flow::IODataList run(
    JITGraphExecutor& executor,
    signal_flow::Graph const& graph,
    signal_flow::IODataList const& input,
    JITGraphExecutor::ChipConfigs const& initial_config,
    bool only_unconnected_output = true) SYMBOL_VISIBLE;

} // namespace grenade::vx::execution
