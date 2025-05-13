#pragma once
#include "grenade/vx/execution/detail/execution_instance_chip_snippet_realtime_executor.h"

#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"
#include "grenade/vx/signal_flow/vertex/madc_readout.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"

namespace grenade::vx::execution::detail {

template <typename T>
void ExecutionInstanceChipSnippetRealtimeExecutor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */, T const& /* data */)
{
	// Specialize for types which are not empty
}

template <>
void ExecutionInstanceChipSnippetRealtimeExecutor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::NeuronView const& /* data */);

template <>
void ExecutionInstanceChipSnippetRealtimeExecutor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::CrossbarL2Input const& /* data */);

template <>
void ExecutionInstanceChipSnippetRealtimeExecutor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::CrossbarL2Output const& /* data */);

template <>
void ExecutionInstanceChipSnippetRealtimeExecutor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::CADCMembraneReadoutView const& /* data */);

template <>
void ExecutionInstanceChipSnippetRealtimeExecutor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::MADCReadoutView const& /* data */);

template <>
void ExecutionInstanceChipSnippetRealtimeExecutor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::PlasticityRule const& /* data */);

} // namespace grenade::vx::execution::detail
