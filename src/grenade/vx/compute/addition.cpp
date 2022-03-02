#include "grenade/vx/compute/addition.h"

#include "grenade/cerealization.h"
#include "grenade/vx/backend/connection.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"
#include "halco/common/cerealization_geometry.h"
#include <cereal/types/vector.hpp>

namespace grenade::vx::compute {

Addition::Addition(std::vector<Int8> const& other) :
    m_graph(), m_input_vertex(), m_other_vertex(), m_output_vertex(), m_other(other)
{
	using namespace halco::hicann_dls::vx;

	auto const instance = coordinate::ExecutionInstance();

	auto const size = other.size();

	m_input_vertex =
	    m_graph.add(vertex::ExternalInput(ConnectionType::DataInt8, size), instance, {});
	m_other_vertex =
	    m_graph.add(vertex::ExternalInput(ConnectionType::DataInt8, size), instance, {});
	auto const vi =
	    m_graph.add(vertex::DataInput(ConnectionType::Int8, size), instance, {m_input_vertex});
	auto const vo =
	    m_graph.add(vertex::DataInput(ConnectionType::Int8, size), instance, {m_other_vertex});
	auto const va = m_graph.add(vertex::Addition(size), instance, {vi, vo});
	m_output_vertex = m_graph.add(vertex::DataOutput(ConnectionType::Int8, size), instance, {va});
}

size_t Addition::input_size() const
{
	return m_other.size();
}

size_t Addition::output_size() const
{
	return input_size();
}

std::vector<std::vector<Int8>> Addition::run(
    std::vector<std::vector<Int8>> const& inputs,
    lola::vx::v2::Chip const& config,
    backend::Connection& connection) const
{
	using namespace halco::hicann_dls::vx;

	JITGraphExecutor::Connections connections;
	connections.insert(std::pair<DLSGlobal, backend::Connection&>(DLSGlobal(), connection));

	JITGraphExecutor::ChipConfigs configs;
	configs.insert(std::pair<DLSGlobal, lola::vx::v2::Chip>(DLSGlobal(), config));

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
	    std::get<vertex::ExternalInput>(m_graph.get_vertex_property(m_input_vertex))
	        .output()
	        .size) {
		throw std::runtime_error("Provided inputs size does not match Addition input size.");
	}

	IODataMap input_map;
	std::vector<TimedDataSequence<std::vector<Int8>>> timed_inputs(inputs.size());
	for (size_t i = 0; i < inputs.size(); ++i) {
		timed_inputs.at(i).resize(1);
		timed_inputs.at(i).at(0).data = inputs.at(i);
	}
	input_map.data[m_input_vertex] = timed_inputs;

	std::vector<TimedDataSequence<std::vector<Int8>>> others(inputs.size());
	for (auto& o : others) {
		o.resize(1);
		// TODO: Think about what to do with timing information
		o.at(0).data = m_other;
	}
	input_map.data[m_other_vertex] = others;

	auto const output_map = JITGraphExecutor::run(m_graph, input_map, connections, configs);

	auto const timed_outputs = std::get<std::vector<TimedDataSequence<std::vector<Int8>>>>(
	    output_map.data.at(m_output_vertex));
	std::vector<std::vector<Int8>> outputs(timed_outputs.size());
	for (size_t i = 0; i < outputs.size(); ++i) {
		assert(timed_outputs.at(i).size() == 1);
		outputs.at(i) = timed_outputs.at(i).at(0).data;
	}
	return outputs;
}

template <typename Archive>
void Addition::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_graph);
	ar(m_input_vertex);
	ar(m_other_vertex);
	ar(m_output_vertex);
	ar(m_other);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::Addition)
CEREAL_CLASS_VERSION(grenade::vx::compute::Addition, 0)
