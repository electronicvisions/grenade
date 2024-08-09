#pragma once
#include <vector>

namespace grenade::common {

template <typename Time, typename T>
struct TimedData
{
	typedef T Data;

	Time time;
	T data;

	TimedData() = default;
	TimedData(Time const& time, T const& data);

	bool operator==(TimedData const& other) const;
	bool operator!=(TimedData const& other) const;
};

template <typename Time, typename T>
using TimedDataSequence = std::vector<TimedData<Time, T>>;

} // namespace grenade::common

#include "grenade/common/timed_data.tcc"
