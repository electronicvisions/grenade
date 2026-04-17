#include "grenade/vx/compute/argmax.h"

#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "grenade/vx/signal_flow/vertex/transformation/argmax.h"

namespace grenade::vx::compute {

ArgMax::ArgMax(size_t size) : m_graph(std::make_shared<grenade::common::Topology>()), m_vertex()
{
	using namespace halco::hicann_dls::vx;

	auto const instance = grenade::common::ExecutionInstanceOnExecutor();

	m_vertex = m_graph->add_vertex(signal_flow::vertex::Transformation(
	    signal_flow::vertex::transformation::ArgMax(size, signal_flow::ConnectionType::Int8),
	    instance));
}

std::vector<std::vector<signal_flow::UInt32>> ArgMax::run(
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
		throw std::runtime_error("Provided inputs size does not match ArgMax input size.");
	}

	grenade::common::InputData input_map;
	std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>> timed_inputs(
	    inputs.size());
	for (size_t i = 0; i < inputs.size(); ++i) {
		timed_inputs.at(i).resize(1);
		// TODO: Think about what to do with timing information
		timed_inputs.at(i).at(0).data = inputs.at(i);
	}
	input_map.ports.set({m_vertex, 0}, signal_flow::vertex::Transformation::Dynamics(timed_inputs));

	auto const output_map = execution::run(executor, m_graph, input_map);

	auto const timed_outputs =
	    std::get<std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>>(
	        dynamic_cast<signal_flow::vertex::Transformation::Results const&>(
	            output_map.ports.get({m_vertex, 0}))
	            .value);
	std::vector<std::vector<signal_flow::UInt32>> outputs(timed_outputs.size());
	for (size_t i = 0; i < outputs.size(); ++i) {
		assert(timed_outputs.at(i).size() == 1);
		outputs.at(i) = timed_outputs.at(i).at(0).data;
	}
	return outputs;
}

size_t ArgMax::input_size() const
{
	return m_graph->get(m_vertex).get_input_ports().at(0).get_channels().size();
}

size_t ArgMax::output_size() const
{
	return 1;
}

} // namespace grenade::vx::compute
