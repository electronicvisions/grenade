#include "grenade/vx/compute/addition.h"

#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/compute/detail/chip_configs.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "grenade/vx/signal_flow/vertex/transformation/addition.h"

namespace grenade::vx::compute {

Addition::Addition(std::vector<signal_flow::Int8> const& other) :
    m_topology(std::make_shared<grenade::common::Topology>()), m_vertex(), m_other(other)
{
	using namespace halco::hicann_dls::vx;

	auto const instance = grenade::common::ExecutionInstanceOnExecutor();

	auto const size = other.size();

	m_vertex = m_topology->add_vertex(signal_flow::vertex::Transformation(
	    signal_flow::vertex::transformation::Addition(2, size), instance));
}

size_t Addition::input_size() const
{
	return m_other.size();
}

size_t Addition::output_size() const
{
	return input_size();
}

std::vector<std::vector<signal_flow::Int8>> Addition::run(
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

	if (batch_entry_size !=
	    m_topology->get(m_vertex).get_output_ports().at(0).get_channels().size()) {
		throw std::runtime_error("Provided inputs size does not match Addition input size.");
	}

	grenade::common::InputData input_map;
	std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>> timed_inputs(
	    inputs.size());
	for (size_t i = 0; i < inputs.size(); ++i) {
		timed_inputs.at(i).resize(1);
		timed_inputs.at(i).at(0).data = inputs.at(i);
	}
	input_map.ports.set({m_vertex, 0}, signal_flow::vertex::Transformation::Dynamics(timed_inputs));

	std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>> others(inputs.size());
	for (auto& o : others) {
		o.resize(1);
		// TODO: Think about what to do with timing information
		o.at(0).data = m_other;
	}
	input_map.ports.set({m_vertex, 1}, signal_flow::vertex::Transformation::Dynamics(others));

	auto const output_map = execution::run(executor, m_topology, input_map);

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

} // namespace grenade::vx::compute
