#include "grenade/vx/compute/argmax.h"

#include "grenade/cerealization.h"
#include "grenade/vx/backend/connection.h"
#include "grenade/vx/config.h"
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
    ChipConfig const& config,
    backend::Connection& connection) const
{
	using namespace halco::hicann_dls::vx;

	JITGraphExecutor::Connections connections;
	connections.insert(std::pair<DLSGlobal, backend::Connection&>(DLSGlobal(), connection));

	JITGraphExecutor::ChipConfigs configs;
	configs.insert(std::pair<DLSGlobal, ChipConfig>(DLSGlobal(), config));

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
	input_map.int8[m_input_vertex] = inputs;

	auto const output_map = JITGraphExecutor::run(m_graph, input_map, connections, configs);

	return output_map.uint32.at(m_output_vertex);
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
