#include "grenade/vx/network/cadc_recording.h"

namespace grenade::vx::network {

CADCRecording::CADCRecording(
    std::vector<Neuron> const& neurons,
    bool placement_on_dram,
    common::EntityOnChip::ChipCoordinate const chip_coordinate) :
    NeuronRecording(neurons, chip_coordinate), placement_on_dram(placement_on_dram)
{}

} // namespace grenade::vx::network
