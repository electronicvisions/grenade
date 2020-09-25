#pragma once
#include <vector>

#include "grenade/vx/graph.h"
#include "grenade/vx/types.h"
#include "hxcomm/vx/connection_variant.h"

namespace cereal {
class access;
} // namespace cereal

namespace grenade::vx {

class ChipConfig;

/**
 * Compute an addition of a constant to given data batches of type Int8.
 */
class ComputeSingleAddition
{
public:
	ComputeSingleAddition() = default;

	/**
	 * Create single Addition compute graph wrapper.
	 * @param other Value to add to given data.
	 */
	ComputeSingleAddition(std::vector<Int8> const& other) SYMBOL_VISIBLE;

	/**
	 * Run given operation.
	 * @param inputs Input values to use
	 * @param config Static chip configuration to be used
	 * @param connection Connection backend to use
	 * @return Resulting values
	 */
	std::vector<std::vector<Int8>> run(
	    std::vector<std::vector<Int8>> const& inputs,
	    ChipConfig const& config,
	    hxcomm::vx::ConnectionVariant& connection) const SYMBOL_VISIBLE;

	size_t input_size() const SYMBOL_VISIBLE;
	size_t output_size() const SYMBOL_VISIBLE;

private:
	Graph m_graph{};
	Graph::vertex_descriptor m_input_vertex{};
	Graph::vertex_descriptor m_other_vertex{};
	Graph::vertex_descriptor m_output_vertex{};
	std::vector<Int8> m_other{};

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx
