#pragma once
#include <vector>

#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/types.h"

namespace cereal {
struct access;
} // namespace cereal

namespace lola::vx::v3 {
class Chip;
} // namespace lola::vx::v3

namespace grenade::vx {

class JITGraphExecutor;

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
	std::vector<std::vector<UInt32>> run(
	    std::vector<std::vector<Int8>> const& inputs,
	    lola::vx::v3::Chip const& config,
	    JITGraphExecutor& executor) const SYMBOL_VISIBLE;

	size_t input_size() const SYMBOL_VISIBLE;
	size_t output_size() const SYMBOL_VISIBLE;

private:
	signal_flow::Graph m_graph;
	signal_flow::Graph::vertex_descriptor m_input_vertex{};
	signal_flow::Graph::vertex_descriptor m_output_vertex{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace compute

} // namespace grenade::vx
