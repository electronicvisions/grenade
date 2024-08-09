#pragma once
#include "grenade/common/timed_data.h"
#include "grenade/vx/common/time.h"

namespace grenade::vx::common {

template <typename T>
using TimedData = grenade::common::TimedData<Time, T>;

template <typename T>
using TimedDataSequence = grenade::common::TimedDataSequence<Time, T>;

} // namespace grenade::vx::common
