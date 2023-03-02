#pragma once
#include <vector>
#include <gtest/gtest_prod.h>

#include "grenade/vx/common/time.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view.h"
#include "halco/common/geometry.h"
#include "haldls/vx/v3/event.h"
#include "haldls/vx/v3/synapse_driver.h"
#include "lola/vx/v3/synapse.h"

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
 * Compute a multiply-accumulate operation with signed weights.
 * Neurons and synapse rows are filled monotonously.
 * If more synapses are needed than fit on a single chip sequential unrolling is used.
 */
class MAC
{
public:
	/**
	 *  weight.
	 */
	struct GENPYBIND(inline_base("*")) Weight
	    : public halco::common::detail::RantWrapper<Weight, int_fast8_t, 63, -63>
	{
		constexpr explicit Weight(intmax_t const val = 0) GENPYBIND(implicit_conversion) :
		    rant_t(val)
		{}

		typedef lola::vx::v3::SynapseMatrix::Weight UnsignedWeight;

		UnsignedWeight toExcitatory() const SYMBOL_VISIBLE;
		UnsignedWeight toInhibitory() const SYMBOL_VISIBLE;
	};

	typedef std::vector<std::vector<Weight>> Weights;
	/** Activations with batch as outer dimension and weight row size as inner dimension. */
	typedef std::vector<std::vector<signal_flow::UInt5>> Activations;

	MAC() = default;

	/**
	 * Create single MAC compute graph wrapper.
	 * @param weights Weight matrix.
	 * @param num_sends Number of times a input activation is sent to the specific row
	 * @param wait_between_events Wait time between input events in FPGA cycles
	 * @param enable_loopback Enable loopback of events with statistic analysis
	 */
	template <typename WeightsT>
	MAC(WeightsT&& weights,
	    size_t num_sends = 1,
	    common::Time wait_between_events = common::Time(25),
	    bool enable_loopback = false);

	/**
	 * Run given set of activations weights given on construction.
	 * @param inputs Input activations to use
	 * @param config Static chip configuration to be used
	 * @param executor Executor backend to use
	 * @return Resulting accumulated membrane potentials
	 */
	std::vector<std::vector<signal_flow::Int8>> run(
	    Activations const& inputs,
	    lola::vx::v3::Chip const& config,
	    execution::JITGraphExecutor& executor) const SYMBOL_VISIBLE;

	size_t input_size() const SYMBOL_VISIBLE;
	size_t output_size() const SYMBOL_VISIBLE;

private:
	/**
	 * Insert a matrix multiplication operation on a synram.
	 * @param graph Graph to insert into
	 * @param weights Weights to use
	 * @param instance Execution instance to place onto
	 * @param hemisphere Hemisphere to place onto
	 * @param crossbar_input_vertex Incoming crossbar input vertex to use
	 * @return Data output vertex to measured membrane potential values
	 */
	static signal_flow::Graph::vertex_descriptor insert_synram(
	    signal_flow::Graph& graph,
	    Weights&& weights,
	    signal_flow::ExecutionInstance const& instance,
	    halco::hicann_dls::vx::v3::HemisphereOnDLS const& hemisphere,
	    signal_flow::Graph::vertex_descriptor crossbar_input_vertex) SYMBOL_VISIBLE;

	bool m_enable_loopback{false};
	signal_flow::Graph m_graph{};

	signal_flow::Graph::vertex_descriptor m_input_vertex{};
	signal_flow::Graph::vertex_descriptor m_output_vertex{};
	Weights m_weights{};

	void build_graph() SYMBOL_VISIBLE;

	size_t m_num_sends{};
	common::Time m_wait_between_events{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace compute

} // namespace grenade::vx

#include "grenade/vx/compute/mac.tcc"
