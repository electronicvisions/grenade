#pragma once
#include "grenade/vx/signal_flow/event.h"

namespace grenade::vx::signal_flow {

template <typename T>
TimedData<T>::TimedData(
    haldls::vx::v3::FPGATime const& fpga_time,
    haldls::vx::v3::ChipTime const& chip_time,
    T const& data) :
    fpga_time(fpga_time), chip_time(chip_time), data(data)
{}

template <typename T>
bool TimedData<T>::operator==(TimedData const& other) const
{
	return fpga_time == other.fpga_time && chip_time == other.chip_time && data == other.data;
}

template <typename T>
bool TimedData<T>::operator!=(TimedData const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::signal_flow
