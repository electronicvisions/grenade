#pragma once
#include <vector>

#include "grenade/vx/graph.h"
#include "grenade/vx/types.h"
#include "hxcomm/vx/connection_variant.h"

namespace grenade::vx {

/**
 * Compute a rectified linear unit operation.
 */
class ComputeSingleReLU
{
public:
	/**
	 * Create single ReLU compute graph wrapper.
	 * @param size Size of operation.
	 */
	ComputeSingleReLU(size_t size) SYMBOL_VISIBLE;

	/**
	 * Run given operation.
	 * @param inputs Input values to use
	 * @param connection Connection backend to use
	 * @return Resulting values
	 */
	std::vector<std::vector<Int8>> run(
	    std::vector<std::vector<Int8>> const& inputs,
	    hxcomm::vx::ConnectionVariant& connection) SYMBOL_VISIBLE;

private:
	Graph m_graph;
	Graph::vertex_descriptor m_input_vertex;
	Graph::vertex_descriptor m_output_vertex;
};

} // namespace grenade::vx
