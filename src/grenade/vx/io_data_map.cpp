#include "grenade/vx/io_data_map.h"

namespace grenade::vx {

IODataMap::IODataMap() :
    uint5(),
    int8(),
    spike_events(),
    spike_event_output(),
    madc_samples(),
    mutex(std::make_unique<std::mutex>())
{}

IODataMap::IODataMap(IODataMap&& other) :
    uint32(std::move(other.uint32)),
    uint5(std::move(other.uint5)),
    int8(std::move(other.int8)),
    spike_events(std::move(other.spike_events)),
    spike_event_output(std::move(other.spike_event_output)),
    madc_samples(std::move(other.madc_samples)),
    mutex(std::move(other.mutex))
{
	other.mutex = std::make_unique<std::mutex>();
}

IODataMap& IODataMap::operator=(IODataMap&& other)
{
	uint32 = std::move(other.uint32);
	uint5 = std::move(other.uint5);
	int8 = std::move(other.int8);
	spike_events = std::move(other.spike_events);
	spike_event_output = std::move(other.spike_event_output);
	madc_samples = std::move(other.madc_samples);
	mutex = std::move(other.mutex);
	other.mutex = std::make_unique<std::mutex>();
	return *this;
}

void IODataMap::merge(IODataMap&& other)
{
	std::unique_lock<std::mutex> lock(*mutex);
	uint32.merge(other.uint32);
	uint5.merge(other.uint5);
	int8.merge(other.int8);
	spike_events.merge(other.spike_events);
	spike_event_output.merge(other.spike_event_output);
	madc_samples.merge(other.madc_samples);
}

void IODataMap::merge(IODataMap& other)
{
	merge(std::forward<IODataMap>(other));
}

void IODataMap::clear()
{
	std::unique_lock<std::mutex> lock(*mutex);
	uint32.clear();
	uint5.clear();
	int8.clear();
	spike_events.clear();
	spike_event_output.clear();
	madc_samples.clear();
}

bool IODataMap::empty() const
{
	return uint32.empty() && uint5.empty() && int8.empty() && spike_events.empty() &&
	       spike_event_output.empty() && madc_samples.empty();
}

namespace {

/**
 * Get batch size via the first entry (any) internal data type.
 */
size_t unsafe_batch_size(IODataMap const& map)
{
	size_t size = 0;
	if (map.uint32.size()) {
		size = map.uint32.begin()->second.size();
	} else if (map.uint5.size()) {
		size = map.uint5.begin()->second.size();
	} else if (map.int8.size()) {
		size = map.int8.begin()->second.size();
	} else if (map.spike_events.size()) {
		size = map.spike_events.begin()->second.size();
	} else if (map.spike_event_output.size()) {
		size = map.spike_event_output.begin()->second.size();
	} else if (map.madc_samples.size()) {
		size = map.madc_samples.begin()->second.size();
	}
	return size;
}

} // namespace

size_t IODataMap::batch_size() const
{
	if (!valid()) {
		throw std::runtime_error("DataMap not valid.");
	}
	return unsafe_batch_size(*this);
}

bool IODataMap::valid() const
{
	size_t size = unsafe_batch_size(*this);
	bool const uint32_value = std::all_of(uint32.cbegin(), uint32.cend(), [size](auto const& list) {
		return list.second.size() == size;
	});
	bool const uint5_value = std::all_of(uint5.cbegin(), uint5.cend(), [size](auto const& list) {
		return list.second.size() == size;
	});
	bool const int8_value = std::all_of(int8.cbegin(), int8.cend(), [size](auto const& list) {
		return list.second.size() == size;
	});
	bool const spike_events_value = std::all_of(
	    spike_events.cbegin(), spike_events.cend(),
	    [size](auto const& list) { return list.second.size() == size; });
	bool const spike_event_output_value = std::all_of(
	    spike_event_output.cbegin(), spike_event_output.cend(),
	    [size](auto const& list) { return list.second.size() == size; });
	bool const madc_samples_value = std::all_of(
	    madc_samples.cbegin(), madc_samples.cend(),
	    [size](auto const& list) { return list.second.size() == size; });
	return uint32_value && uint5_value && int8_value && spike_events_value &&
	       spike_event_output_value && madc_samples_value;
}


ConstantReferenceIODataMap::ConstantReferenceIODataMap() :
    uint32(), uint5(), int8(), spike_events(), spike_event_output(), madc_samples()
{}

void ConstantReferenceIODataMap::clear()
{
	uint32.clear();
	uint5.clear();
	int8.clear();
	spike_events.clear();
	spike_event_output.clear();
	madc_samples.clear();
}

} // namespace grenade::vx
