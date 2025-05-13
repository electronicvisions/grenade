#include "grenade/vx/network/cadc_recording.h"

namespace grenade::vx::network {

CADCRecording::CADCRecording(
    std::vector<Neuron> const& neurons,
    bool placement_on_dram,
    common::EntityOnChip::ChipOnExecutor const chip_on_executor) :
    NeuronRecording(neurons, chip_on_executor), placement_on_dram(placement_on_dram)
{}

} // namespace grenade::vx::network
