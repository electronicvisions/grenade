#include "grenade/vx/network/run.h"

#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/signal_flow/execution_instance.h"
#include "halco/hicann-dls/vx/v3/chip.h"

namespace grenade::vx::network {

using namespace halco::hicann_dls::vx::v3;

signal_flow::IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    lola::vx::v3::Chip const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& inputs)
{
	signal_flow::ExecutionInstancePlaybackHooks empty;
	return run(connection, config, network_graph, inputs, empty);
}

signal_flow::IODataMap run(
    grenade::vx::execution::backend::Connection& connection,
    lola::vx::v3::Chip const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& inputs)
{
	signal_flow::ExecutionInstancePlaybackHooks empty;
	return run(connection, config, network_graph, inputs, empty);
}

signal_flow::IODataMap run(
    grenade::vx::execution::JITGraphExecutor& executor,
    lola::vx::v3::Chip const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& inputs)
{
	signal_flow::ExecutionInstancePlaybackHooks empty;
	return run(executor, config, network_graph, inputs, empty);
}

signal_flow::IODataMap run(
    grenade::vx::execution::JITGraphExecutor& executor,
    lola::vx::v3::Chip const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& inputs,
    signal_flow::ExecutionInstancePlaybackHooks& playback_hooks)
{
	grenade::vx::execution::JITGraphExecutor::ChipConfigs configs;
	configs.insert(std::pair<signal_flow::ExecutionInstance, lola::vx::v3::Chip>(
	    signal_flow::ExecutionInstance(), config));

	grenade::vx::execution::JITGraphExecutor::PlaybackHooks playback_hooks_map;
	playback_hooks_map.insert(
	    std::pair<signal_flow::ExecutionInstance, signal_flow::ExecutionInstancePlaybackHooks>(
	        signal_flow::ExecutionInstance(), std::move(playback_hooks)));

	auto ret = grenade::vx::execution::run(
	    executor, network_graph.get_graph(), inputs, configs, playback_hooks_map);

	return ret;
}

signal_flow::IODataMap run(
    grenade::vx::execution::backend::Connection& connection,
    lola::vx::v3::Chip const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& inputs,
    signal_flow::ExecutionInstancePlaybackHooks& playback_hooks)
{
	std::map<DLSGlobal, grenade::vx::execution::backend::Connection> connections;
	connections.emplace(DLSGlobal(), std::move(connection));
	grenade::vx::execution::JITGraphExecutor executor(std::move(connections));

	auto ret = run(executor, config, network_graph, inputs, playback_hooks);

	connection = std::move(executor.release_connections().at(DLSGlobal()));
	return ret;
}

signal_flow::IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    lola::vx::v3::Chip const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& inputs,
    signal_flow::ExecutionInstancePlaybackHooks& playback_hooks)
{
	grenade::vx::execution::backend::Connection backend_connection(std::move(connection));

	auto ret = run(backend_connection, config, network_graph, inputs, playback_hooks);
	connection = backend_connection.release();
	return ret;
}

} // namespace grenade::vx::network
