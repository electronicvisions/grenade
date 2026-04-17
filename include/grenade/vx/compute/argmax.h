#pragma once
#include <vector>

#include "grenade/common/topology.h"
#include "grenade/vx/signal_flow/types.h"
#include "lola/vx/v3/chip.h"

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx {

namespace execution {
class JITGraphExecutor;
} // namespace execution

namespace compute {

/**
 * Compute a argmax operation.
 * The returned index type is 32bit unsigned integer.
 */
class ArgMax
{
public:
	ArgMax() = default;

	/**
	 * Create single ArgMax compute graph wrapper.
	 * @param size Size of operation.
	 */
	ArgMax(size_t size) SYMBOL_VISIBLE;

	/**
	 * Run given operation.
	 * @param inputs Input values to use
	 * @param config Static chip configuration to be used
	 * @param executor Executor backend to use
	 * @return Resulting values
	 */
	std::vector<std::vector<signal_flow::UInt32>> run(
	    std::vector<std::vector<signal_flow::Int8>> const& inputs,
	    lola::vx::v3::Chip const& config,
	    execution::JITGraphExecutor& executor) const SYMBOL_VISIBLE;

	size_t input_size() const SYMBOL_VISIBLE;
	size_t output_size() const SYMBOL_VISIBLE;

private:
	std::shared_ptr<grenade::common::Topology> m_graph;
	grenade::common::VertexOnTopology m_vertex{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace compute

} // namespace grenade::vx
