#pragma once
#include <vector>

#include "grenade/common/topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/signal_flow/types.h"

namespace cereal {
struct access;
} // namespace cereal

namespace lola::vx::v3 {
class Chip;
} // namespace lola::vx::v3

namespace grenade::vx {

namespace execution {
class JITGraphExecutor;
} // namespace execution

namespace compute {

/**
 * Compute an addition of a constant to given data batches of type signal_flow::Int8.
 */
class Addition
{
public:
	Addition() = default;

	/**
	 * Create single Addition compute topology wrapper.
	 * @param other Value to add to given data.
	 */
	Addition(std::vector<signal_flow::Int8> const& other) SYMBOL_VISIBLE;

	/**
	 * Run given operation.
	 * @param inputs Input values to use
	 * @param executor Executor backend to use
	 * @return Resulting values
	 */
	std::vector<std::vector<signal_flow::Int8>> run(
	    std::vector<std::vector<signal_flow::Int8>> const& inputs,
	    lola::vx::v3::Chip const& config,
	    execution::JITGraphExecutor& executor) const SYMBOL_VISIBLE;

	size_t input_size() const SYMBOL_VISIBLE;
	size_t output_size() const SYMBOL_VISIBLE;

private:
	std::shared_ptr<grenade::common::Topology> m_topology{};
	grenade::common::VertexOnTopology m_vertex{};
	std::vector<signal_flow::Int8> m_other{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace compute

} // namespace grenade::vx
