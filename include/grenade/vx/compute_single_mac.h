#pragma once
#include <vector>
#include <gtest/gtest_prod.h>

#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "grenade/vx/vertex/synapse_array_view.h"
#include "haldls/vx/v2/event.h"
#include "haldls/vx/v2/synapse_driver.h"
#include "haldls/vx/v2/timer.h"
#include "hxcomm/vx/connection_variant.h"
#include "lola/vx/v2/synapse.h"

namespace cereal {
class access;
} // namespace cereal

class ComputeSingleMAC_get_spike_label_Test;
class ComputeSingleMAC_generate_input_events_Test;

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
	typedef std::vector<std::vector<lola::vx::v2::SynapseMatrix::Weight>> Weights;
	typedef std::vector<haldls::vx::v2::SynapseDriverConfig::RowMode> RowModes;
	/** Activations with batch as outer dimension and weight row size as inner dimension. */
	typedef std::vector<std::vector<UInt5>> Activations;

	ComputeSingleMAC() = default;

	/**
	 * Create single MAC compute graph wrapper.
	 * @param weights Weight matrix.
	 * @param row_modes Modes of rows in weight matrix (typically exc./inh. for pos./neg.)
	 * @param num_sends Number of times a input activation is sent to the specific row
	 * @param wait_between_events Wait time between input events in FPGA cycles
	 * @param enable_loopback Enable loopback of events with statistic analysis
	 */
	template <typename WeightsT, typename RowModesT>
	ComputeSingleMAC(
	    WeightsT&& weights,
	    RowModesT&& row_modes,
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

private:
	FRIEND_TEST(::ComputeSingleMAC, get_spike_label);
	FRIEND_TEST(::ComputeSingleMAC, generate_input_events);

	/**
	 * Get spike label value from location and activation value.
	 * @param row Synapse row to send to
	 * @param value Activation value to send
	 * @return SpikeLabel value if activation value is larger than zero
	 */
	static std::optional<haldls::vx::v2::SpikeLabel> get_spike_label(
	    halco::hicann_dls::vx::v2::SynapseRowOnDLS const& row, UInt5 const value) SYMBOL_VISIBLE;

	/**
	 * Insert a matrix multiplication operation on a synram.
	 * @param graph Graph to insert into
	 * @param weights Weights to use
	 * @param row_modes Row modes (exc./inh.) to use
	 * @param instance Execution instance to place onto
	 * @param hemisphere Hemisphere to place onto
	 * @param crossbar_input_vertex Incoming crossbar input vertex to use
	 * @return Data output vertex to measured membrane potential values
	 */
	static Graph::vertex_descriptor insert_synram(
	    Graph& graph,
	    Weights&& weights,
	    RowModes&& row_modes,
	    coordinate::ExecutionInstance const& instance,
	    halco::hicann_dls::vx::v2::HemisphereOnDLS const& hemisphere,
	    Graph::vertex_descriptor crossbar_input_vertex) SYMBOL_VISIBLE;

	struct SynramHandle
	{
		Graph::vertex_descriptor input_vertex;
		size_t input_size;
		size_t input_offset;
		halco::hicann_dls::vx::v2::HemisphereOnDLS hemisphere;
	};

	static IODataMap generate_input_events(
	    Activations const& inputs,
	    std::vector<SynramHandle> const& synram_handles,
	    size_t num_sends,
	    haldls::vx::v2::Timer::Value wait_between_events) SYMBOL_VISIBLE;

	bool m_enable_loopback{false};
	Graph m_graph{};

	std::vector<SynramHandle> m_synram_handles{};
	std::vector<Graph::vertex_descriptor> m_output_vertices{};
	Weights m_weights{};
	RowModes m_row_modes{};

	void build_graph() SYMBOL_VISIBLE;

	size_t input_size() const;
	size_t output_size() const;

	size_t m_num_sends{};
	haldls::vx::v2::Timer::Value m_wait_between_events{};

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx

#include "grenade/vx/compute_single_mac.tcc"
