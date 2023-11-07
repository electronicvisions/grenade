#pragma once
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include <type_traits>

namespace grenade::vx::signal_flow::vertex {

template <typename InputVertex>
bool EntityOnChip::supports_input_from(
    InputVertex const& input, std::optional<PortRestriction> const&) const
{
	if constexpr (std::is_base_of_v<EntityOnChip, std::decay_t<InputVertex>>) {
		return input.chip_coordinate == chip_coordinate;
	} else {
		return true;
	}
}

} // namespace grenade::vx::signal_flow::vertex
