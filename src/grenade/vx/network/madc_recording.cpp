#include "grenade/vx/network/madc_recording.h"

namespace grenade::vx::network {

MADCRecording::MADCRecording(
    std::vector<Neuron> const& neurons,
    common::EntityOnChip::ChipOnExecutor const chip_on_executor) :
    NeuronRecording(neurons, chip_on_executor)
{}

} // namespace grenade::vx::network
