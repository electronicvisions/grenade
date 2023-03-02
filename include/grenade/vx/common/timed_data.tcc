#pragma once
#include "grenade/vx/common/timed_data.h"

namespace grenade::vx::common {

template <typename T>
TimedData<T>::TimedData(Time const& time, T const& data) : time(time), data(data)
{}

template <typename T>
bool TimedData<T>::operator==(TimedData const& other) const
{
	return time == other.time && data == other.data;
}

template <typename T>
bool TimedData<T>::operator!=(TimedData const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::common
