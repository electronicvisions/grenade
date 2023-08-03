#pragma once
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <optional>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow {

struct PortRestriction;

namespace vertex {

/**
 * Entity on chip mixin carrying chip id information.
 */
struct EntityOnChip
{
	typedef halco::hicann_dls::vx::v3::DLSGlobal ChipCoordinate;

	explicit EntityOnChip(ChipCoordinate const& coordinate_chip = ChipCoordinate()) SYMBOL_VISIBLE;

	ChipCoordinate get_coordinate_chip() const SYMBOL_VISIBLE;

	bool operator==(EntityOnChip const& other) const SYMBOL_VISIBLE;
	bool operator!=(EntityOnChip const& other) const SYMBOL_VISIBLE;

	/**
	 * Whether entity on chip supports input from given vertex.
	 * This is true exactly if the input vertex resides on the same chip or is not derived from
	 * EntityOnChip. When the derived-from vertex employs further restrictions this base-class
	 * method is to be checked within the derived-from method.
	 */
	template <typename InputVertex>
	bool supports_input_from(
	    InputVertex const& input, std::optional<PortRestriction> const& port_restriction) const;

	friend std::ostream& operator<<(std::ostream& os, EntityOnChip const& entity) SYMBOL_VISIBLE;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);

	ChipCoordinate m_coordinate_chip;
};

} // namespace vertex

} // namespace grenade::vx::signal_flow

#include "grenade/vx/signal_flow/vertex/entity_on_chip.tcc"
