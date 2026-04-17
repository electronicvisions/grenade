#include "grenade/vx/compute/relu.h"

#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/compute/detail/chip_configs.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "grenade/vx/signal_flow/vertex/transformation/relu.h"

namespace grenade::vx::compute {

ReLU::ReLU(size_t size) : m_graph(std::make_shared<grenade::common::Topology>()), m_vertex()
{
	using namespace halco::hicann_dls::vx;

	auto const instance = grenade::common::ExecutionInstanceOnExecutor();

	m_vertex = m_graph->add_vertex(signal_flow::vertex::Transformation(
	    signal_flow::vertex::transformation::ReLU(size), instance));
}

std::vector<std::vector<signal_flow::Int8>> ReLU::run(
    std::vector<std::vector<signal_flow::Int8>> const& inputs,
    lola::vx::v3::Chip const&,
    execution::JITGraphExecutor& executor) const
{
	using namespace halco::hicann_dls::vx;

	if (inputs.size() == 0) {
		throw std::runtime_error("Provided inputs are empty.");
	}

	size_t const batch_entry_size = inputs.front().size();
	for (auto const& batch_entry : inputs) {
		if (batch_entry.size() != batch_entry_size) {
			throw std::runtime_error("Provided batch entries don't share a common size.");
		}
	}

	if (batch_entry_size != input_size()) {
		throw std::runtime_error("Provided inputs size does not match ReLU input size.");
	}

	grenade::common::InputData input_data;
	std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>> timed_inputs(
	    inputs.size());
	for (size_t i = 0; i < inputs.size(); ++i) {
		timed_inputs.at(i).resize(1);
		// TODO: Think about what to do with timing information
		timed_inputs.at(i).at(0).data = inputs.at(i);
	}
	input_data.ports.set(
	    {m_vertex, 0}, signal_flow::vertex::Transformation::Dynamics(timed_inputs));

	auto const output_map = execution::run(executor, m_graph, input_data);

	auto const timed_outputs =
	    std::get<std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(
	        dynamic_cast<signal_flow::vertex::Transformation::Results const&>(
	            output_map.ports.get({m_vertex, 0}))
	            .value);
	std::vector<std::vector<signal_flow::Int8>> outputs(timed_outputs.size());
	for (size_t i = 0; i < outputs.size(); ++i) {
		assert(timed_outputs.at(i).size() == 1);
		outputs.at(i) = timed_outputs.at(i).at(0).data;
	}
	return outputs;
}

size_t ReLU::input_size() const
{
	return m_graph->get(m_vertex).get_input_ports().at(0).get_channels().size();
}

size_t ReLU::output_size() const
{
	return input_size();
}

} // namespace grenade::vx::compute
