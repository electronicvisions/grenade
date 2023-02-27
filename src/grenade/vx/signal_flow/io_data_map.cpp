#include "grenade/vx/signal_flow/io_data_map.h"

namespace grenade::vx::signal_flow {

IODataMap::IODataMap() :
    data(), runtime(), execution_time_info(), mutex(std::make_unique<std::mutex>())
{}

IODataMap::IODataMap(IODataMap&& other) :
    data(std::move(other.data)),
    runtime(std::move(other.runtime)),
    execution_time_info(std::move(other.execution_time_info)),
    mutex(std::move(other.mutex))
{
	other.mutex = std::make_unique<std::mutex>();
}

IODataMap& IODataMap::operator=(IODataMap&& other)
{
	data = std::move(other.data);
	runtime = std::move(other.runtime);
	execution_time_info = std::move(other.execution_time_info);
	mutex = std::move(other.mutex);
	other.mutex = std::make_unique<std::mutex>();
	return *this;
}

void IODataMap::merge(IODataMap&& other)
{
	std::unique_lock<std::mutex> lock(*mutex);
	data.merge(other.data);
	runtime.merge(other.runtime);
	if (execution_time_info) {
		execution_time_info->merge(*(other.execution_time_info));
	} else {
		execution_time_info = std::move(other.execution_time_info);
		other.execution_time_info.reset();
	}
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
	execution_time_info.reset();
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

bool IODataMap::is_match(Entry const& entry, signal_flow::Port const& port)
{
	auto const check_shape = [&](auto const& d) {
		typedef std::remove_cvref_t<decltype(d)> Data;
		typedef hate::type_list<
		    std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::UInt32>>>,
		    std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::UInt5>>>,
		    std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::Int8>>>>
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

	if ((port.type == signal_flow::ConnectionType::DataUInt32) &&
	    !std::holds_alternative<
	        std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::UInt32>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::DataUInt5) &&
	    !std::holds_alternative<
	        std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::UInt5>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::DataInt8) &&
	    !std::holds_alternative<
	        std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::Int8>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::DataTimedSpikeSequence) &&
	    !std::holds_alternative<std::vector<signal_flow::TimedSpikeSequence>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::DataTimedSpikeFromChipSequence) &&
	    !std::holds_alternative<std::vector<signal_flow::TimedSpikeFromChipSequence>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::DataTimedMADCSampleFromChipSequence) &&
	    !std::holds_alternative<std::vector<signal_flow::TimedMADCSampleFromChipSequence>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::UInt32) &&
	    !std::holds_alternative<
	        std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::UInt32>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::UInt5) &&
	    !std::holds_alternative<
	        std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::UInt5>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::Int8) &&
	    !std::holds_alternative<
	        std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::Int8>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::TimedSpikeSequence) &&
	    !std::holds_alternative<std::vector<signal_flow::TimedSpikeSequence>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::TimedSpikeFromChipSequence) &&
	    !std::holds_alternative<std::vector<signal_flow::TimedSpikeFromChipSequence>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::TimedMADCSampleFromChipSequence) &&
	    !std::holds_alternative<std::vector<signal_flow::TimedMADCSampleFromChipSequence>>(entry)) {
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

} // namespace grenade::vx::signal_flow
