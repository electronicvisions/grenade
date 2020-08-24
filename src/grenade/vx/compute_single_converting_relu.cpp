#include "grenade/vx/compute_single_converting_relu.h"

#include "grenade/cerealization.h"
#include "grenade/vx/config.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"

namespace grenade::vx {

ComputeSingleConvertingReLU::ComputeSingleConvertingReLU(size_t size, uint32_t shift) :
    m_graph(), m_input_vertex(), m_output_vertex()
{
	using namespace halco::hicann_dls::vx;

	auto const instance = coordinate::ExecutionInstance();

	m_input_vertex =
	    m_graph.add(vertex::ExternalInput(ConnectionType::DataOutputInt8, size), instance, {});
	auto const v2 =
	    m_graph.add(vertex::DataInput(ConnectionType::Int8, size), instance, {m_input_vertex});
	auto const v3 = m_graph.add(vertex::ConvertingReLU(size, shift), instance, {v2});
	m_output_vertex = m_graph.add(vertex::DataOutput(ConnectionType::UInt5, size), instance, {v3});
}

std::vector<std::vector<UInt5>> ComputeSingleConvertingReLU::run(
    std::vector<std::vector<Int8>> const& inputs,
    ChipConfig const& config,
    hxcomm::vx::ConnectionVariant& connection)
{
	using namespace halco::hicann_dls::vx;

	JITGraphExecutor::Connections connections;
	connections.insert(
	    std::pair<DLSGlobal, hxcomm::vx::ConnectionVariant&>(DLSGlobal(), connection));

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

	if (batch_entry_size !=
	    std::get<vertex::ExternalInput>(m_graph.get_vertex_property(m_input_vertex))
	        .output()
	        .size) {
		throw std::runtime_error("Provided inputs size does not match ConvertingReLU input size.");
	}

	IODataMap input_map;
	input_map.int8[m_input_vertex] = inputs;

	auto const output_map = JITGraphExecutor::run(m_graph, input_map, connections, configs);

	return output_map.uint5.at(m_output_vertex);
}
template <typename Archive>
void ComputeSingleConvertingReLU::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_graph);
	ar(m_input_vertex);
	ar(m_output_vertex);
}

} // namespace grenade::vx

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::ComputeSingleConvertingReLU)
CEREAL_CLASS_VERSION(grenade::vx::ComputeSingleConvertingReLU, 0)
