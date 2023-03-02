#include "grenade/vx/compute/relu.h"

#include "grenade/cerealization.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/signal_flow/execution_instance.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input.h"
#include "grenade/vx/signal_flow/io_data_map.h"

namespace grenade::vx::compute {

ReLU::ReLU(size_t size) : m_graph(), m_input_vertex(), m_output_vertex()
{
	using namespace halco::hicann_dls::vx;

	auto const instance = signal_flow::ExecutionInstance();

	m_input_vertex = m_graph.add(
	    signal_flow::vertex::ExternalInput(signal_flow::ConnectionType::DataInt8, size), instance,
	    {});
	auto const v2 = m_graph.add(
	    signal_flow::vertex::DataInput(signal_flow::ConnectionType::Int8, size), instance,
	    {m_input_vertex});
	auto const v3 = m_graph.add(signal_flow::vertex::ReLU(size), instance, {v2});
	m_output_vertex = m_graph.add(
	    signal_flow::vertex::DataOutput(signal_flow::ConnectionType::Int8, size), instance, {v3});
}

std::vector<std::vector<signal_flow::Int8>> ReLU::run(
    std::vector<std::vector<signal_flow::Int8>> const& inputs,
    lola::vx::v3::Chip const& config,
    execution::JITGraphExecutor& executor) const
{
	using namespace halco::hicann_dls::vx;

	execution::JITGraphExecutor::ChipConfigs configs;
	configs.insert(std::pair<signal_flow::ExecutionInstance, lola::vx::v3::Chip>(
	    signal_flow::ExecutionInstance(), config));

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

	signal_flow::IODataMap input_map;
	std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>> timed_inputs(
	    inputs.size());
	for (size_t i = 0; i < inputs.size(); ++i) {
		timed_inputs.at(i).resize(1);
		// TODO: Think about what to do with timing information
		timed_inputs.at(i).at(0).data = inputs.at(i);
	}
	input_map.data[m_input_vertex] = timed_inputs;

	auto const output_map = execution::run(executor, m_graph, input_map, configs);

	auto const timed_outputs =
	    std::get<std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(
	        output_map.data.at(m_output_vertex));
	std::vector<std::vector<signal_flow::Int8>> outputs(timed_outputs.size());
	for (size_t i = 0; i < outputs.size(); ++i) {
		assert(timed_outputs.at(i).size() == 1);
		outputs.at(i) = timed_outputs.at(i).at(0).data;
	}
	return outputs;
}

size_t ReLU::input_size() const
{
	return std::get<signal_flow::vertex::ExternalInput>(m_graph.get_vertex_property(m_input_vertex))
	    .output()
	    .size;
}

size_t ReLU::output_size() const
{
	return input_size();
}

template <typename Archive>
void ReLU::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_graph);
	ar(m_input_vertex);
	ar(m_output_vertex);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::ReLU)
CEREAL_CLASS_VERSION(grenade::vx::compute::ReLU, 0)
