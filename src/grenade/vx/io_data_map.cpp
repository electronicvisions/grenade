#include "grenade/vx/io_data_map.h"

namespace grenade::vx {

IODataMap::IODataMap() : data(), runtime(), mutex(std::make_unique<std::mutex>()) {}

IODataMap::IODataMap(IODataMap&& other) :
    data(std::move(other.data)), runtime(std::move(other.runtime)), mutex(std::move(other.mutex))
{
	other.mutex = std::make_unique<std::mutex>();
}

IODataMap& IODataMap::operator=(IODataMap&& other)
{
	data = std::move(other.data);
	runtime = std::move(other.runtime);
	mutex = std::move(other.mutex);
	other.mutex = std::make_unique<std::mutex>();
	return *this;
}

void IODataMap::merge(IODataMap&& other)
{
	std::unique_lock<std::mutex> lock(*mutex);
	data.merge(other.data);
	runtime.merge(other.runtime);
}

void IODataMap::merge(IODataMap& other)
{
	merge(std::forward<IODataMap>(other));
}

void IODataMap::clear()
{
	std::unique_lock<std::mutex> lock(*mutex);
	data.clear();
	runtime.clear();
}

bool IODataMap::empty() const
{
	return data.empty() && runtime.empty();
}

namespace {

/**
 * Get batch size via the first entry (any) internal data type.
 */
size_t unsafe_batch_size(IODataMap const& map)
{
	size_t size = 0;
	if (map.runtime.size()) {
		if (map.runtime.begin()->second.size()) {
			size = map.runtime.begin()->second.size();
		}
	} else if (map.data.size()) {
		size = std::visit([](auto const& d) { return d.size(); }, map.data.begin()->second);
	}
	return size;
}

/**
 * Get validity of map given arbitrary batch size.
 */
bool unsafe_valid(IODataMap const& map, size_t const size)
{
	bool const runtime_value = std::all_of(
	    map.runtime.cbegin(), map.runtime.cend(),
	    [size](auto const& ei) { return (ei.second.size() == size) || (ei.second.size() == 0); });
	bool const data_value =
	    std::all_of(map.data.cbegin(), map.data.cend(), [size](auto const& list) {
		    return std::visit([size](auto const& d) { return d.size() == size; }, list.second);
	    });
	return runtime_value && data_value;
}

} // namespace

size_t IODataMap::batch_size() const
{
	size_t const size = unsafe_batch_size(*this);
	if (!unsafe_valid(*this, size)) {
		throw std::runtime_error("DataMap not valid.");
	}
	return size;
}

bool IODataMap::valid() const
{
	return unsafe_valid(*this, unsafe_batch_size(*this));
}

bool IODataMap::is_match(Entry const& entry, Port const& port)
{
	auto const check_shape = [&](auto const& d) {
		typedef std::remove_cvref_t<decltype(d)> Data;
		typedef hate::type_list<
		    std::vector<TimedDataSequence<std::vector<UInt32>>>,
		    std::vector<TimedDataSequence<std::vector<UInt5>>>,
		    std::vector<TimedDataSequence<std::vector<Int8>>>>
		    MatrixData;
		if constexpr (hate::is_in_type_list<Data, MatrixData>::value) {
			if (!d.empty()) {
				if (!d.front().empty()) {
					if (d.front().at(0).data.size() != port.size) {
						return false;
					}
				}
			}
		}
		return true;
	};
	if (!std::visit(check_shape, entry)) {
		return false;
	}

	if ((port.type == ConnectionType::DataUInt32) &&
	    !std::holds_alternative<std::vector<TimedDataSequence<std::vector<UInt32>>>>(entry)) {
		return false;
	} else if (
	    (port.type == ConnectionType::DataUInt5) &&
	    !std::holds_alternative<std::vector<TimedDataSequence<std::vector<UInt5>>>>(entry)) {
		return false;
	} else if (
	    (port.type == ConnectionType::DataInt8) &&
	    !std::holds_alternative<std::vector<TimedDataSequence<std::vector<Int8>>>>(entry)) {
		return false;
	} else if (
	    (port.type == ConnectionType::DataTimedSpikeSequence) &&
	    !std::holds_alternative<std::vector<TimedSpikeSequence>>(entry)) {
		return false;
	} else if (
	    (port.type == ConnectionType::DataTimedSpikeFromChipSequence) &&
	    !std::holds_alternative<std::vector<TimedSpikeFromChipSequence>>(entry)) {
		return false;
	} else if (
	    (port.type == ConnectionType::DataTimedMADCSampleFromChipSequence) &&
	    !std::holds_alternative<std::vector<TimedMADCSampleFromChipSequence>>(entry)) {
		return false;
	} else if (
	    (port.type == ConnectionType::UInt32) &&
	    !std::holds_alternative<std::vector<TimedDataSequence<std::vector<UInt32>>>>(entry)) {
		return false;
	} else if (
	    (port.type == ConnectionType::UInt5) &&
	    !std::holds_alternative<std::vector<TimedDataSequence<std::vector<UInt5>>>>(entry)) {
		return false;
	} else if (
	    (port.type == ConnectionType::Int8) &&
	    !std::holds_alternative<std::vector<TimedDataSequence<std::vector<Int8>>>>(entry)) {
		return false;
	} else if (
	    (port.type == ConnectionType::TimedSpikeSequence) &&
	    !std::holds_alternative<std::vector<TimedSpikeSequence>>(entry)) {
		return false;
	} else if (
	    (port.type == ConnectionType::TimedSpikeFromChipSequence) &&
	    !std::holds_alternative<std::vector<TimedSpikeFromChipSequence>>(entry)) {
		return false;
	} else if (
	    (port.type == ConnectionType::TimedMADCSampleFromChipSequence) &&
	    !std::holds_alternative<std::vector<TimedMADCSampleFromChipSequence>>(entry)) {
		return false;
	}
	return true;
}


ConstantReferenceIODataMap::ConstantReferenceIODataMap() : data(), runtime() {}

void ConstantReferenceIODataMap::clear()
{
	data.clear();
	runtime.clear();
}

} // namespace grenade::vx
