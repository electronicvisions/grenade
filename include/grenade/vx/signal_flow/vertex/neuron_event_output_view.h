#pragma once
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/port.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/visibility.h"
#include <array>
#include <iosfwd>
#include <map>
#include <optional>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow {

struct PortRestriction;

namespace vertex {

struct NeuronView;

/**
 * A view of neuron event outputs into the routing crossbar.
 */
struct NeuronEventOutputView : public EntityOnChip
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef std::vector<halco::hicann_dls::vx::v3::NeuronColumnOnDLS> Columns;
	typedef halco::hicann_dls::vx::v3::NeuronRowOnDLS Row;
	typedef std::map<Row, std::vector<Columns>> Neurons;

	NeuronEventOutputView() = default;

	/**
	 * Construct NeuronEventOutputView with specified neurons.
	 * @param neurons Incoming neurons
	 * @param chip_on_executor Coordinate of chip to use
	 */
	NeuronEventOutputView(
	    Neurons const& neurons,
	    ChipOnExecutor const& chip_on_executor = ChipOnExecutor()) SYMBOL_VISIBLE;

	Neurons const& get_neurons() const SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	/**
	 * Input ports are organized in the same order as the specified neuron views.
	 */
	std::vector<Port> inputs() const SYMBOL_VISIBLE;

	/**
	 * Output ports are sorted neuron event output channels.
	 */
	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, NeuronEventOutputView const& config)
	    SYMBOL_VISIBLE;

	bool supports_input_from(
	    NeuronView const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

	bool operator==(NeuronEventOutputView const& other) const SYMBOL_VISIBLE;
	bool operator!=(NeuronEventOutputView const& other) const SYMBOL_VISIBLE;

private:
	Neurons m_neurons{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex

} // grenade::vx::signal_flow
