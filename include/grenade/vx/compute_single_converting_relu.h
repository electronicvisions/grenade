#pragma once
#include <vector>

#include "grenade/vx/graph.h"
#include "grenade/vx/types.h"
#include "hxcomm/vx/connection_variant.h"

namespace cereal {
class access;
} // namespace cereal

namespace grenade::vx {

struct ChipConfig;

/**
 * Compute a rectified linear unit operation converting from Int8 to UInt5.
 */
class ComputeSingleConvertingReLU
{
public:
	ComputeSingleConvertingReLU() = default;

	/**
	 * Create single ConvertingReLU compute graph wrapper.
	 * @param size Size of operation
	 * @param shift Power-of-two (bitshift) scaling parameter before clamping, i.e. saturation to
	 *        UInt5 value range
	 */
	ComputeSingleConvertingReLU(size_t size, uint32_t shift) SYMBOL_VISIBLE;

	/**
	 * Run given operation.
	 * @param inputs Input values to use
	 * @param config Static chip configuration to be used
	 * @param connection Connection backend to use
	 * @return Resulting values
	 */
	std::vector<std::vector<UInt5>> run(
	    std::vector<std::vector<Int8>> const& inputs,
	    ChipConfig const& config,
	    hxcomm::vx::ConnectionVariant& connection) const SYMBOL_VISIBLE;

private:
	Graph m_graph{};
	Graph::vertex_descriptor m_input_vertex{};
	Graph::vertex_descriptor m_output_vertex{};

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx