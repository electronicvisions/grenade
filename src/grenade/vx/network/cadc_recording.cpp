#include "grenade/vx/network/cadc_recording.h"

namespace grenade::vx::network {

CADCRecording::CADCRecording(
    std::vector<Neuron> const& neurons,
    common::EntityOnChip::ChipCoordinate const chip_coordinate) :
    NeuronRecording(neurons, chip_coordinate)
{}

} // namespace grenade::vx::network
