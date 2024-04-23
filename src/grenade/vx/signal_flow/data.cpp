#include "grenade/vx/signal_flow/data.h"

#include <stdexcept>

namespace grenade::vx::signal_flow {

Data::Data() : data() {}

Data::Data(Data&& other) : data(std::move(other.data)) {}

Data& Data::operator=(Data&& other)
{
	data = std::move(other.data);
	return *this;
}

void Data::merge(Data&& other)
{
	data.merge(other.data);
}

void Data::merge(Data& other)
{
	merge(std::forward<Data>(other));
}

void Data::clear()
{
	data.clear();
}

bool Data::empty() const
{
	return data.empty();
}

namespace {

/**
 * Get batch size via the first entry (any) internal data type.
 */
size_t unsafe_batch_size(Data const& map)
{
	if (map.data.empty()) {
		return 0;
	}
	return std::visit([](auto const& d) { return d.size(); }, map.data.begin()->second);
}

/**
 * Get validity of map given arbitrary batch size.
 */
bool unsafe_valid(Data const& map, size_t const size)
{
	return std::all_of(map.data.cbegin(), map.data.cend(), [size](auto const& list) {
		return std::visit([size](auto const& d) { return d.size() == size; }, list.second);
	});
}

} // namespace

size_t Data::batch_size() const
{
	size_t const size = unsafe_batch_size(*this);
	if (!unsafe_valid(*this, size)) {
		throw std::runtime_error("Data is not valid.");
	}
	return size;
}

bool Data::valid() const
{
	return unsafe_valid(*this, unsafe_batch_size(*this));
}

bool Data::is_match(Entry const& entry, signal_flow::Port const& port)
{
	auto const check_shape = [&](auto const& d) {
		typedef std::remove_cvref_t<decltype(d)> Data;
		typedef hate::type_list<
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>
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
	        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::DataUInt5) &&
	    !std::holds_alternative<
	        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::DataInt8) &&
	    !std::holds_alternative<
	        std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::DataTimedSpikeToChipSequence) &&
	    !std::holds_alternative<std::vector<signal_flow::TimedSpikeToChipSequence>>(entry)) {
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
	        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::UInt5) &&
	    !std::holds_alternative<
	        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::Int8) &&
	    !std::holds_alternative<
	        std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(entry)) {
		return false;
	} else if (
	    (port.type == signal_flow::ConnectionType::TimedSpikeToChipSequence) &&
	    !std::holds_alternative<std::vector<signal_flow::TimedSpikeToChipSequence>>(entry)) {
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

} // namespace grenade::vx::signal_flow
