#include "grenade/vx/data_map.h"

namespace grenade::vx {

DataMap::DataMap() :
    int8(), spike_events(), spike_event_output(), mutex(std::make_unique<std::mutex>())
{}

DataMap::DataMap(DataMap&& other) :
    int8(std::move(other.int8)),
    spike_events(std::move(other.spike_events)),
    spike_event_output(std::move(other.spike_event_output)),
    mutex(std::move(other.mutex))
{
	other.mutex = std::make_unique<std::mutex>();
}

DataMap& DataMap::operator=(DataMap&& other)
{
	int8 = std::move(other.int8);
	spike_events = std::move(other.spike_events);
	spike_event_output = std::move(other.spike_event_output);
	mutex = std::move(other.mutex);
	other.mutex = std::make_unique<std::mutex>();
	return *this;
}

void DataMap::merge(DataMap&& other)
{
	std::unique_lock<std::mutex> lock(*mutex);
	int8.merge(other.int8);
	spike_events.merge(other.spike_events);
	spike_event_output.merge(other.spike_event_output);
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
	spike_event_output.clear();
}

bool DataMap::empty() const
{
	return int8.empty() && spike_events.empty() && spike_event_output.empty();
}

namespace {

/**
 * Get batch size via the first entry (any) internal data type.
 */
size_t unsafe_batch_size(DataMap const& map)
{
	size_t size = 0;
	if (map.int8.size()) {
		size = map.int8.begin()->second.size();
	} else if (map.spike_events.size()) {
		size = map.spike_events.begin()->second.size();
	} else if (map.spike_event_output.size()) {
		size = map.spike_event_output.begin()->second.size();
	}
	return size;
}

} // namespace

size_t DataMap::batch_size() const
{
	if (!valid()) {
		throw std::runtime_error("DataMap not valid.");
	}
	return unsafe_batch_size(*this);
}

bool DataMap::valid() const
{
	size_t size = unsafe_batch_size(*this);
	bool const int8_value = std::all_of(int8.cbegin(), int8.cend(), [size](auto const& list) {
		return list.second.size() == size;
	});
	bool const spike_events_value = std::all_of(
	    spike_events.cbegin(), spike_events.cend(),
	    [size](auto const& list) { return list.second.size() == size; });
	bool const spike_event_output_value = std::all_of(
	    spike_event_output.cbegin(), spike_event_output.cend(),
	    [size](auto const& list) { return list.second.size() == size; });
	return int8_value && spike_events_value && spike_event_output_value;
}

} // namespace grenade::vx
