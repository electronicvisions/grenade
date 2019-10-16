#pragma once
#include <vector>
#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "grenade/vx/vertex/synapse_array_view.h"
#include "hxcomm/vx/connection_variant.h"
#include "lola/vx/synapse.h"

namespace grenade::vx {

class ChipConfig;

/**
 * Compute a multiply-accumulate operation.
 * Neurons and synapse rows are filled monotonously.
 * If more synapses are needed than fit on a single chip sequential unrolling is used.
 */
class ComputeSingleMAC
{
public:
	typedef std::vector<std::vector<lola::vx::SynapseMatrix::Weight>> Weights;
	typedef std::vector<haldls::vx::SynapseDriverConfig::RowMode> RowModes;
	typedef std::vector<UInt5> Activations;

	/**
	 * Create single MAC compute graph wrapper.
	 * @param weights Weight matrix.
	 * @param row_modes Modes of rows in weight matrix (typically exc./inh. for pos./neg.)
	 * @param config Static chip configuration to be used
	 * @param num_sends Number of times a input activation is sent to the specific row
	 */
	ComputeSingleMAC(
	    Weights const& weights,
	    RowModes const& row_modes,
	    ChipConfig const& config,
	    size_t num_sends = 1) SYMBOL_VISIBLE;

	/**
	 * Run given set of activations weights given on construction.
	 * @param inputs Input activations to use
	 * @param connection Connection backend to use
	 * @return Resulting accumulated membrane potentials
	 */
	std::vector<Int8> run(Activations const& inputs, hxcomm::vx::ConnectionVariant& connection)
	    SYMBOL_VISIBLE;

private:
	Graph m_graph;
	std::vector<Graph::vertex_descriptor> m_input_vertices;
	std::vector<Graph::vertex_descriptor> m_output_vertices;
	std::vector<size_t> m_input_sizes;
	std::vector<size_t> m_input_offsets;
	Weights m_weights;
	RowModes m_row_modes;
	JITGraphExecutor::ConfigMap m_config_map;

	size_t input_size() const;
	size_t output_size() const;
};

} // namespace grenade::vx
