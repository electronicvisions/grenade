#pragma once
#include "grenade/common/timed_data.h"

namespace grenade::common {

template <typename Time, typename T>
TimedData<Time, T>::TimedData(Time const& time, T const& data) : time(time), data(data)
{}

template <typename Time, typename T>
bool TimedData<Time, T>::operator==(TimedData const& other) const
{
	return time == other.time && data == other.data;
}

template <typename Time, typename T>
bool TimedData<Time, T>::operator!=(TimedData const& other) const
{
	return !(*this == other);
}

} // namespace grenade::common
