#include "grenade/vx/network/run.h"

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/jit_graph_executor.h"
#include "halco/hicann-dls/vx/v2/chip.h"

namespace grenade::vx::network {

using namespace halco::hicann_dls::vx::v2;

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
    hxcomm::vx::ConnectionVariant& connection,
    ChipConfig const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs,
    ExecutionInstancePlaybackHooks& playback_hooks)
{
	JITGraphExecutor::Connections connections;
	backend::Connection backend_connection(std::move(connection));
	connections.insert(std::pair<DLSGlobal, backend::Connection&>(DLSGlobal(), backend_connection));

	JITGraphExecutor::ChipConfigs configs;
	configs.insert(std::pair<DLSGlobal, ChipConfig>(DLSGlobal(), config));

	JITGraphExecutor::PlaybackHooks playback_hooks_map;
	playback_hooks_map.insert(
	    std::pair<coordinate::ExecutionInstance, ExecutionInstancePlaybackHooks>(
	        coordinate::ExecutionInstance(), std::move(playback_hooks)));

	auto ret = JITGraphExecutor::run(
	    network_graph.get_graph(), inputs, connections, configs, playback_hooks_map);
	connection = connections.at(DLSGlobal()).release();
	return ret;
}

} // namespace grenade::vx::network
