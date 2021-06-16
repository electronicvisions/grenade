#include "grenade/vx/network/run.h"

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
	connections.insert(
	    std::pair<DLSGlobal, hxcomm::vx::ConnectionVariant&>(DLSGlobal(), connection));

	JITGraphExecutor::ChipConfigs configs;
	configs.insert(std::pair<DLSGlobal, ChipConfig>(DLSGlobal(), config));

	JITGraphExecutor::PlaybackHooks playback_hooks_map;
	playback_hooks_map.insert(
	    std::pair<coordinate::ExecutionInstance, ExecutionInstancePlaybackHooks>(
	        coordinate::ExecutionInstance(), std::move(playback_hooks)));

	return JITGraphExecutor::run(
	    network_graph.get_graph(), inputs, connections, configs, playback_hooks_map);
}

} // namespace grenade::vx::network
