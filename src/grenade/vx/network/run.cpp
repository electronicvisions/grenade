#include "grenade/vx/network/run.h"

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
	JITGraphExecutor::Connections connections;
	connections.insert(
	    std::pair<DLSGlobal, hxcomm::vx::ConnectionVariant&>(DLSGlobal(), connection));

	JITGraphExecutor::ChipConfigs configs;
	configs.insert(std::pair<DLSGlobal, ChipConfig>(DLSGlobal(), config));

	return JITGraphExecutor::run(network_graph.graph, inputs, connections, configs);
}

} // namespace grenade::vx::network
