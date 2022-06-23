#include "grenade/vx/network/run.h"

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/jit_graph_executor.h"
#include "halco/hicann-dls/vx/v3/chip.h"

namespace grenade::vx::network {

using namespace halco::hicann_dls::vx::v3;

IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    lola::vx::v3::Chip const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs)
{
	ExecutionInstancePlaybackHooks empty;
	return run(connection, config, network_graph, inputs, empty);
}

IODataMap run(
    backend::Connection& connection,
    lola::vx::v3::Chip const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs)
{
	ExecutionInstancePlaybackHooks empty;
	return run(connection, config, network_graph, inputs, empty);
}

IODataMap run(
    JITGraphExecutor& executor,
    lola::vx::v3::Chip const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs)
{
	ExecutionInstancePlaybackHooks empty;
	return run(executor, config, network_graph, inputs, empty);
}

IODataMap run(
    JITGraphExecutor& executor,
    lola::vx::v3::Chip const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs,
    ExecutionInstancePlaybackHooks& playback_hooks)
{
	JITGraphExecutor::ChipConfigs configs;
	configs.insert(std::pair<coordinate::ExecutionInstance, lola::vx::v3::Chip>(
	    coordinate::ExecutionInstance(), config));

	JITGraphExecutor::PlaybackHooks playback_hooks_map;
	playback_hooks_map.insert(
	    std::pair<coordinate::ExecutionInstance, ExecutionInstancePlaybackHooks>(
	        coordinate::ExecutionInstance(), std::move(playback_hooks)));

	auto ret =
	    grenade::vx::run(executor, network_graph.get_graph(), inputs, configs, playback_hooks_map);

	return ret;
}

IODataMap run(
    backend::Connection& connection,
    lola::vx::v3::Chip const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs,
    ExecutionInstancePlaybackHooks& playback_hooks)
{
	std::map<DLSGlobal, backend::Connection> connections;
	connections.emplace(DLSGlobal(), std::move(connection));
	JITGraphExecutor executor(std::move(connections));

	auto ret = run(executor, config, network_graph, inputs, playback_hooks);

	connection = std::move(executor.release_connections().at(DLSGlobal()));
	return ret;
}

IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    lola::vx::v3::Chip const& config,
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
