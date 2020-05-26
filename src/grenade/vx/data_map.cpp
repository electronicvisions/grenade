#include "grenade/vx/data_map.h"

namespace grenade::vx {

void DataMap::merge(DataMap&& other)
{
	int8.merge(other.int8);
	spike_events.merge(other.spike_events);
}

void DataMap::merge(DataMap& other)
{
	merge(std::forward<DataMap>(other));
}

void DataMap::clear()
{
	int8.clear();
	spike_events.clear();
}

} // namespace grenade::vx
