#include "grenade/vx/network/run.h"

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/jit_graph_executor.h"
#include "halco/hicann-dls/vx/v3/chip.h"

namespace grenade::vx::network {

using namespace halco::hicann_dls::vx::v3;

IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    ChipConfig const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs)
{
	ExecutionInstancePlaybackHooks empty;
	return run(connection, config, network_graph, inputs, empty);
}

IODataMap run(
    backend::Connection& connection,
    ChipConfig const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs)
{
	ExecutionInstancePlaybackHooks empty;
	return run(connection, config, network_graph, inputs, empty);
}

IODataMap run(
    backend::Connection& connection,
    ChipConfig const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs,
    ExecutionInstancePlaybackHooks& playback_hooks)
{
	JITGraphExecutor::Connections connections;
	connections.insert(std::pair<DLSGlobal, backend::Connection&>(DLSGlobal(), connection));

	JITGraphExecutor::ChipConfigs configs;
	configs.insert(std::pair<DLSGlobal, ChipConfig>(DLSGlobal(), config));

	JITGraphExecutor::PlaybackHooks playback_hooks_map;
	playback_hooks_map.insert(
	    std::pair<coordinate::ExecutionInstance, ExecutionInstancePlaybackHooks>(
	        coordinate::ExecutionInstance(), std::move(playback_hooks)));

	auto ret = JITGraphExecutor::run(
	    network_graph.get_graph(), inputs, connections, configs, playback_hooks_map);
	return ret;
}

IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    ChipConfig const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs,
    ExecutionInstancePlaybackHooks& playback_hooks)
{
	backend::Connection backend_connection(std::move(connection));

	auto ret = run(backend_connection, config, network_graph, inputs, playback_hooks);
	connection = backend_connection.release();
	return ret;
}

} // namespace grenade::vx::network
