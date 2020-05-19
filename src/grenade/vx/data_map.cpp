#include "grenade/vx/data_map.h"

namespace grenade::vx {

DataMap::DataMap() : int8(), spike_events(), mutex(std::make_unique<std::mutex>()) {}

DataMap::DataMap(DataMap&& other) :
    int8(std::move(other.int8)),
    spike_events(std::move(other.spike_events)),
    mutex(std::move(other.mutex))
{
	other.mutex = std::make_unique<std::mutex>();
}

DataMap& DataMap::operator=(DataMap&& other)
{
	int8 = std::move(other.int8);
	spike_events = std::move(other.spike_events);
	mutex = std::move(other.mutex);
	other.mutex = std::make_unique<std::mutex>();
	return *this;
}

void DataMap::merge(DataMap&& other)
{
	std::unique_lock<std::mutex> lock(*mutex);
	int8.merge(other.int8);
	spike_events.merge(other.spike_events);
}

void DataMap::merge(DataMap& other)
{
	merge(std::forward<DataMap>(other));
}

void DataMap::clear()
{
	std::unique_lock<std::mutex> lock(*mutex);
	int8.clear();
	spike_events.clear();
}

} // namespace grenade::vx
