#include "grenade/vx/network/run.h"

#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "halco/hicann-dls/vx/v3/chip.h"

namespace grenade::vx::network {

using namespace halco::hicann_dls::vx::v3;

signal_flow::IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input)
{
	execution::JITGraphExecutor::PlaybackHooks empty;
	return run(connection, config, network_graph, input, empty);
}

std::vector<signal_flow::IODataMap> run(
    hxcomm::vx::ConnectionVariant& connection,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs)
{
	execution::JITGraphExecutor::PlaybackHooks empty;
	return run(connection, configs, network_graphs, inputs, empty);
}

signal_flow::IODataMap run(
    grenade::vx::execution::backend::Connection& connection,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input)
{
	execution::JITGraphExecutor::PlaybackHooks empty;
	return run(connection, config, network_graph, input, empty);
}

std::vector<signal_flow::IODataMap> run(
    grenade::vx::execution::backend::Connection& connection,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs)
{
	execution::JITGraphExecutor::PlaybackHooks empty;
	return run(connection, configs, network_graphs, inputs, empty);
}

signal_flow::IODataMap run(
    grenade::vx::execution::JITGraphExecutor& executor,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input)
{
	execution::JITGraphExecutor::PlaybackHooks empty;
	return run(executor, config, network_graph, input, empty);
}

std::vector<signal_flow::IODataMap> run(
    grenade::vx::execution::JITGraphExecutor& executor,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs)
{
	execution::JITGraphExecutor::PlaybackHooks empty;
	return run(executor, configs, network_graphs, inputs, empty);
}

signal_flow::IODataMap run(
    grenade::vx::execution::JITGraphExecutor& executor,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks)
{
	return grenade::vx::execution::run(
	    executor, network_graph.get_graph(), input, config, playback_hooks);
}

std::vector<signal_flow::IODataMap> run(
    grenade::vx::execution::JITGraphExecutor& executor,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks)
{
	std::vector<std::reference_wrapper<signal_flow::Graph const>> graphs;
	for (size_t i = 0; i < network_graphs.size(); i++) {
		graphs.push_back(network_graphs[i].get().get_graph());
	}
	return grenade::vx::execution::run(executor, graphs, inputs, configs, playback_hooks);
}

signal_flow::IODataMap run(
    grenade::vx::execution::backend::Connection& connection,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks)
{
	std::map<DLSGlobal, grenade::vx::execution::backend::Connection> connections;
	connections.emplace(DLSGlobal(), std::move(connection));
	grenade::vx::execution::JITGraphExecutor executor(std::move(connections));

	auto ret = run(executor, config, network_graph, input, playback_hooks);

	connection = std::move(executor.release_connections().at(DLSGlobal()));
	return ret;
}

std::vector<signal_flow::IODataMap> run(
    grenade::vx::execution::backend::Connection& connection,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks)
{
	std::map<DLSGlobal, grenade::vx::execution::backend::Connection> connections;
	connections.emplace(DLSGlobal(), std::move(connection));
	grenade::vx::execution::JITGraphExecutor executor(std::move(connections));

	auto ret = run(executor, configs, network_graphs, inputs, playback_hooks);

	connection = std::move(executor.release_connections().at(DLSGlobal()));
	return ret;
}

signal_flow::IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks)
{
	grenade::vx::execution::backend::Connection backend_connection(std::move(connection));

	auto ret = run(backend_connection, config, network_graph, input, playback_hooks);
	connection = backend_connection.release();
	return ret;
}

std::vector<signal_flow::IODataMap> run(
    hxcomm::vx::ConnectionVariant& connection,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks)
{
	grenade::vx::execution::backend::Connection backend_connection(std::move(connection));

	auto ret = run(backend_connection, configs, network_graphs, inputs, playback_hooks);
	connection = backend_connection.release();
	return ret;
}

} // namespace grenade::vx::network
