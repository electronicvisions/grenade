#pragma once
#include <vector>

#include "grenade/vx/graph.h"
#include "grenade/vx/types.h"

namespace cereal {
class access;
} // namespace cereal

namespace lola::vx::v2 {
class Chip;
} // namespace lola::vx::v2

namespace grenade::vx {

namespace backend {
class Connection;
} // namespace backend

namespace compute {

/**
 * Compute a rectified linear unit operation.
 */
class ReLU
{
public:
	ReLU() = default;

	/**
	 * Create single ReLU compute graph wrapper.
	 * @param size Size of operation.
	 */
	ReLU(size_t size) SYMBOL_VISIBLE;

	/**
	 * Run given operation.
	 * @param inputs Input values to use
	 * @param config Static chip configuration to be used
	 * @param connection Connection backend to use
	 * @return Resulting values
	 */
	std::vector<std::vector<Int8>> run(
	    std::vector<std::vector<Int8>> const& inputs,
	    lola::vx::v2::Chip const& config,
	    backend::Connection& connection) const SYMBOL_VISIBLE;

	size_t input_size() const SYMBOL_VISIBLE;
	size_t output_size() const SYMBOL_VISIBLE;

private:
	Graph m_graph{};
	Graph::vertex_descriptor m_input_vertex{};
	Graph::vertex_descriptor m_output_vertex{};

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace compute

} // namespace grenade::vx
