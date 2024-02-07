#include "grenade/vx/signal_flow/output_data.h"

#include <stdexcept>

namespace grenade::vx::signal_flow {

OutputData::OutputData() :
    Data(),
    execution_time_info(),
    read_ppu_symbols(),
    pre_execution_chips(),
    mutex(std::make_unique<std::mutex>())
{}

OutputData::OutputData(OutputData&& other) :
    Data(static_cast<Data&&>(other)),
    execution_time_info(std::move(other.execution_time_info)),
    read_ppu_symbols(std::move(other.read_ppu_symbols)),
    pre_execution_chips(std::move(other.pre_execution_chips)),
    mutex(std::move(other.mutex))
{
	other.mutex = std::make_unique<std::mutex>();
}

OutputData& OutputData::operator=(OutputData&& other)
{
	static_cast<Data&>(*this).operator=(static_cast<Data&&>(other));
	execution_time_info = std::move(other.execution_time_info);
	read_ppu_symbols = std::move(other.read_ppu_symbols);
	pre_execution_chips = std::move(other.pre_execution_chips);
	mutex = std::move(other.mutex);
	other.mutex = std::make_unique<std::mutex>();
	return *this;
}

void OutputData::merge(OutputData&& other)
{
	std::unique_lock<std::mutex> lock(*mutex);
	static_cast<Data&>(*this).merge(other);
	if (!other.read_ppu_symbols.empty()) {
		if (read_ppu_symbols.empty()) {
			read_ppu_symbols = std::move(other.read_ppu_symbols);
			other.read_ppu_symbols.clear();
		} else {
			if (read_ppu_symbols.size() != other.read_ppu_symbols.size()) {
				throw std::runtime_error(
				    "Read PPU symbols sizes need to match for OutputData::merge(), but are (" +
				    std::to_string(read_ppu_symbols.size()) + ") vs. (" +
				    std::to_string(other.read_ppu_symbols.size()) + ").");
			}
			for (size_t i = 0; i < read_ppu_symbols.size(); ++i) {
				read_ppu_symbols.at(i).merge(other.read_ppu_symbols.at(i));
			}
		}
	}
	pre_execution_chips.merge(other.pre_execution_chips);
	if (execution_time_info && other.execution_time_info) {
		execution_time_info->merge(*(other.execution_time_info));
	} else {
		execution_time_info = std::move(other.execution_time_info);
		other.execution_time_info.reset();
	}
}

void OutputData::merge(OutputData& other)
{
	merge(std::forward<OutputData>(other));
}

void OutputData::clear()
{
	std::unique_lock<std::mutex> lock(*mutex);
	static_cast<Data&>(*this).clear();
	execution_time_info.reset();
	read_ppu_symbols.clear();
	pre_execution_chips.clear();
}

bool OutputData::empty() const
{
	return static_cast<Data const&>(*this).empty() && read_ppu_symbols.empty() &&
	       pre_execution_chips.empty();
}

size_t OutputData::batch_size() const
{
	size_t const size = static_cast<Data const&>(*this).batch_size();
	if (!((read_ppu_symbols.size() == size) || read_ppu_symbols.empty() || data.empty())) {
		throw std::runtime_error("OutputData not valid.");
	}
	return data.empty() ? read_ppu_symbols.size() : size;
}

bool OutputData::valid() const
{
	bool const data_value = static_cast<Data const&>(*this).valid();
	if (!data_value) {
		return false;
	}
	size_t const size = static_cast<Data const&>(*this).batch_size();
	return (read_ppu_symbols.size() == size) || read_ppu_symbols.empty() ||
	       static_cast<Data const&>(*this).empty();
}

} // namespace grenade::vx::signal_flow
