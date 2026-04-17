#pragma once
#include <vector>

#include "grenade/common/topology.h"
#include "grenade/common/vertex_on_topology.h"
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
 * Compute a rectified linear unit operation converting from signal_flow::Int8 to
 * signal_flow::UInt5.
 */
class ConvertingReLU
{
public:
	ConvertingReLU() = default;

	/**
	 * Create single ConvertingReLU compute graph wrapper.
	 * @param size Size of operation
	 * @param shift Power-of-two (bitshift) scaling parameter before clamping, i.e. saturation to
	 *        signal_flow::UInt5 value range
	 */
	ConvertingReLU(size_t size, uint32_t shift) SYMBOL_VISIBLE;

	/**
	 * Run given operation.
	 * @param inputs Input values to use
	 * @param config Static chip configuration to be used
	 * @param executor Executor backend to use
	 * @return Resulting values
	 */
	std::vector<std::vector<signal_flow::UInt5>> run(
	    std::vector<std::vector<signal_flow::Int8>> const& inputs,
	    lola::vx::v3::Chip const& config,
	    execution::JITGraphExecutor& executor) const SYMBOL_VISIBLE;

	size_t input_size() const SYMBOL_VISIBLE;
	size_t output_size() const SYMBOL_VISIBLE;

private:
	std::shared_ptr<grenade::common::Topology> m_graph{};
	grenade::common::VertexOnTopology m_vertex{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace compute

} // namespace grenade::vx
