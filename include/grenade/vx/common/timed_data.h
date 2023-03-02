#pragma once
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"
#include <vector>

namespace grenade::vx::common {

template <typename T>
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

template <typename T>
using TimedDataSequence = std::vector<TimedData<T>>;

} // namespace grenade::vx::common

#include "grenade/vx/common/timed_data.tcc"
