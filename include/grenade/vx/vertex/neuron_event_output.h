#pragma once
#include <array>
#include <optional>
#include <ostream>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "halco/hicann-dls/vx/v2/neuron.h"
#include "hate/visibility.h"

namespace cereal {
class access;
} // namespace cereal

namespace grenade::vx {

struct PortRestriction;

namespace vertex {

struct NeuronView;

/**
 * A view of neuron event outputs into the routing crossbar.
 */
struct NeuronEventOutputView
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef std::vector<halco::hicann_dls::vx::v2::NeuronColumnOnDLS> Columns;
	typedef halco::hicann_dls::vx::v2::NeuronRowOnDLS Row;

	NeuronEventOutputView() = default;

	/**
	 * Construct NeuronEventOutputView with specified neurons.
	 * @param columns Neuron columns
	 * @param row Neuron row
	 */
	NeuronEventOutputView(Columns const& columns, Row const& row) SYMBOL_VISIBLE;

	Columns const& get_columns() const SYMBOL_VISIBLE;
	Row const& get_row() const SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, NeuronEventOutputView const& config)
	    SYMBOL_VISIBLE;

	bool supports_input_from(
	    NeuronView const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

	bool operator==(NeuronEventOutputView const& other) const SYMBOL_VISIBLE;
	bool operator!=(NeuronEventOutputView const& other) const SYMBOL_VISIBLE;

private:
	Columns m_columns{};
	Row m_row{};

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex

} // grenade::vx
