#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"

namespace grenade::vx::signal_flow::vertex {

EntityOnChip::EntityOnChip(ChipOnExecutor const& chip_on_executor) :
    common::EntityOnChip(chip_on_executor)
{}

} // namespace grenade::vx::signal_flow::vertex
