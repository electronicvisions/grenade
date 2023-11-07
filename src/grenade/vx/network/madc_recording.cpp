#include "grenade/vx/network/madc_recording.h"

namespace grenade::vx::network {

MADCRecording::MADCRecording(
    std::vector<Neuron> const& neurons,
    common::EntityOnChip::ChipCoordinate const chip_coordinate) :
    NeuronRecording(neurons, chip_coordinate)
{}

} // namespace grenade::vx::network
