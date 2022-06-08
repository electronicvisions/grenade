#include "grenade/vx/compute/argmax.h"

#include "grenade/cerealization.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"

namespace grenade::vx::compute {

ArgMax::ArgMax(size_t size) : m_graph(), m_input_vertex(), m_output_vertex()
{
	using namespace halco::hicann_dls::vx;

	auto const instance = coordinate::ExecutionInstance();

	m_input_vertex =
	    m_graph.add(vertex::ExternalInput(ConnectionType::DataInt8, size), instance, {});
	auto const v2 =
	    m_graph.add(vertex::DataInput(ConnectionType::Int8, size), instance, {m_input_vertex});
	auto const v3 = m_graph.add(vertex::ArgMax(size, ConnectionType::Int8), instance, {v2});
	m_output_vertex = m_graph.add(vertex::DataOutput(ConnectionType::UInt32, 1), instance, {v3});
}

std::vector<std::vector<UInt32>> ArgMax::run(
    std::vector<std::vector<Int8>> const& inputs,
    lola::vx::v3::Chip const& config,
    JITGraphExecutor& executor) const
{
	using namespace halco::hicann_dls::vx;

	JITGraphExecutor::ChipConfigs configs;
	configs.insert(std::pair<coordinate::ExecutionInstance, lola::vx::v3::Chip>(
	    coordinate::ExecutionInstance(), config));

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

	IODataMap input_map;
	std::vector<TimedDataSequence<std::vector<Int8>>> timed_inputs(inputs.size());
	for (size_t i = 0; i < inputs.size(); ++i) {
		timed_inputs.at(i).resize(1);
		// TODO: Think about what to do with timing information
		timed_inputs.at(i).at(0).data = inputs.at(i);
	}
	input_map.data[m_input_vertex] = timed_inputs;

	auto const output_map = executor.run(m_graph, input_map, configs);

	auto const timed_outputs = std::get<std::vector<TimedDataSequence<std::vector<UInt32>>>>(
	    output_map.data.at(m_output_vertex));
	std::vector<std::vector<UInt32>> outputs(timed_outputs.size());
	for (size_t i = 0; i < outputs.size(); ++i) {
		assert(timed_outputs.at(i).size() == 1);
		outputs.at(i) = timed_outputs.at(i).at(0).data;
	}
	return outputs;
}

size_t ArgMax::input_size() const
{
	return std::get<vertex::ExternalInput>(m_graph.get_vertex_property(m_input_vertex))
	    .output()
	    .size;
}

size_t ArgMax::output_size() const
{
	return 1;
}

template <typename Archive>
void ArgMax::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_graph);
	ar(m_input_vertex);
	ar(m_output_vertex);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::ArgMax)
CEREAL_CLASS_VERSION(grenade::vx::compute::ArgMax, 0)
