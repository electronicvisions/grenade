#pragma once
#include <vector>
#include <gtest/gtest_prod.h>

#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "grenade/vx/vertex/synapse_array_view.h"
#include "halco/common/geometry.h"
#include "haldls/vx/v2/event.h"
#include "haldls/vx/v2/synapse_driver.h"
#include "haldls/vx/v2/timer.h"
#include "hxcomm/vx/connection_variant.h"
#include "lola/vx/v2/synapse.h"

namespace cereal {
class access;
} // namespace cereal

namespace grenade::vx {

class ChipConfig;

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

		typedef lola::vx::v2::SynapseMatrix::Weight UnsignedWeight;

		UnsignedWeight toExcitatory() const SYMBOL_VISIBLE;
		UnsignedWeight toInhibitory() const SYMBOL_VISIBLE;
	};

	typedef std::vector<std::vector<Weight>> Weights;
	/** Activations with batch as outer dimension and weight row size as inner dimension. */
	typedef std::vector<std::vector<UInt5>> Activations;

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
	    haldls::vx::v2::Timer::Value wait_between_events = haldls::vx::v2::Timer::Value(25),
	    bool enable_loopback = true);

	/**
	 * Run given set of activations weights given on construction.
	 * @param inputs Input activations to use
	 * @param config Static chip configuration to be used
	 * @param connection Connection backend to use
	 * @return Resulting accumulated membrane potentials
	 */
	std::vector<std::vector<Int8>> run(
	    Activations const& inputs,
	    ChipConfig const& config,
	    hxcomm::vx::ConnectionVariant& connection) const SYMBOL_VISIBLE;

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
	static Graph::vertex_descriptor insert_synram(
	    Graph& graph,
	    Weights&& weights,
	    coordinate::ExecutionInstance const& instance,
	    halco::hicann_dls::vx::v2::HemisphereOnDLS const& hemisphere,
	    Graph::vertex_descriptor crossbar_input_vertex) SYMBOL_VISIBLE;

	bool m_enable_loopback{false};
	Graph m_graph{};

	Graph::vertex_descriptor m_input_vertex{};
	Graph::vertex_descriptor m_output_vertex{};
	Weights m_weights{};

	void build_graph() SYMBOL_VISIBLE;

	size_t m_num_sends{};
	haldls::vx::v2::Timer::Value m_wait_between_events{};

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace compute

} // namespace grenade::vx

#include "grenade/vx/compute/mac.tcc"
