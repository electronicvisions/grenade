#pragma once
#include "grenade/vx/common/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/visibility.h"
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
struct EntityOnChip : public common::EntityOnChip
{
	explicit EntityOnChip(ChipCoordinate const& coordinate_chip = ChipCoordinate()) SYMBOL_VISIBLE;

	/**
	 * Whether entity on chip supports input from given vertex.
	 * This is true exactly if the input vertex resides on the same chip or is not derived from
	 * EntityOnChip. When the derived-from vertex employs further restrictions this base-class
	 * method is to be checked within the derived-from method.
	 */
	template <typename InputVertex>
	bool supports_input_from(
	    InputVertex const& input, std::optional<PortRestriction> const& port_restriction) const;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace vertex

} // namespace grenade::vx::signal_flow

#include "grenade/vx/signal_flow/vertex/entity_on_chip.tcc"
