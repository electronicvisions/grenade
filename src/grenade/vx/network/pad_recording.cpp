#include "grenade/vx/network/pad_recording.h"

namespace grenade::vx::network {

PadRecording::Source::Source(NeuronRecording::Neuron const& neuron, bool enable_buffered) :
    neuron(neuron), enable_buffered(enable_buffered)
{}

bool PadRecording::Source::operator==(Source const& other) const
{
	return neuron == other.neuron && enable_buffered == other.enable_buffered;
}

bool PadRecording::Source::operator!=(Source const& other) const
{
	return !(*this == other);
}


PadRecording::PadRecording(
    Recordings const& recordings, common::EntityOnChip::ChipCoordinate const chip_coordinate) :
    common::EntityOnChip(chip_coordinate), recordings(recordings)
{}

bool PadRecording::operator==(PadRecording const& other) const
{
	return recordings == other.recordings && static_cast<common::EntityOnChip const&>(*this) ==
	                                             static_cast<common::EntityOnChip const&>(other);
}

bool PadRecording::operator!=(PadRecording const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::network
