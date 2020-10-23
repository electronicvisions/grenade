#pragma once
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "halco/hicann-dls/vx/v2/neuron.h"
#include "hate/visibility.h"
#include <array>
#include <cstddef>
#include <iosfwd>
#include <optional>

namespace cereal {
class access;
} // namespace cereal

namespace grenade::vx {

struct PortRestriction;

namespace vertex {

struct NeuronView;

/**
 * Readout of membrane voltages via the MADC.
 */
struct MADCMembraneReadoutView
{
	constexpr static bool can_connect_different_execution_instances = false;

	typedef halco::hicann_dls::vx::v2::AtomicNeuronOnDLS Coord;

	MADCMembraneReadoutView() = default;

	/**
	 * Construct MADCMembraneReadoutView.
	 * @param columns Neuron to read out
	 */
	explicit MADCMembraneReadoutView(Coord const& coord);

	Coord const& get_coord() const SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, MADCMembraneReadoutView const& config)
	    SYMBOL_VISIBLE;

	bool supports_input_from(
	    NeuronView const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

	bool operator==(MADCMembraneReadoutView const& other) const SYMBOL_VISIBLE;
	bool operator!=(MADCMembraneReadoutView const& other) const SYMBOL_VISIBLE;

private:
	Coord m_coord{};

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex

} // grenade::vx
