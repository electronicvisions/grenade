#include "grenade/vx/network/run.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "halco/hicann-dls/vx/v3/chip.h"

namespace grenade::vx::network {

using namespace halco::hicann_dls::vx::v3;

signal_flow::OutputData run(
    grenade::vx::execution::JITGraphExecutor& executor,
    NetworkGraph const& network_graph,
    execution::JITGraphExecutor::ChipConfigs const& config,
    signal_flow::InputData const& input,
    execution::JITGraphExecutor::Hooks&& hooks)
{
	return grenade::vx::execution::run(
	    executor, network_graph.get_graph(), config, input, std::move(hooks));
}

std::vector<signal_flow::OutputData> run(
    grenade::vx::execution::JITGraphExecutor& executor,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<signal_flow::InputData const>> const& inputs,
    execution::JITGraphExecutor::Hooks&& hooks)
{
	std::vector<std::reference_wrapper<signal_flow::Graph const>> graphs;
	for (size_t i = 0; i < network_graphs.size(); i++) {
		graphs.push_back(network_graphs[i].get().get_graph());
	}
	return grenade::vx::execution::run(executor, graphs, configs, inputs, std::move(hooks));
}

} // namespace grenade::vx::network
