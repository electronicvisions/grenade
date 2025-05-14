#include "grenade/vx/signal_flow/output_data.h"

#include <algorithm>
#include <stdexcept>

namespace grenade::vx::signal_flow {

OutputData::OutputData() :
    snippets(),
    execution_time_info(),
    execution_health_info(),
    read_ppu_symbols(),
    mutex(std::make_unique<std::mutex>())
{}

OutputData::OutputData(OutputData&& other) :
    snippets(std::move(other.snippets)),
    execution_time_info(std::move(other.execution_time_info)),
    execution_health_info(std::move(other.execution_health_info)),
    read_ppu_symbols(std::move(other.read_ppu_symbols)),
    mutex(std::move(other.mutex))
{
	other.mutex = std::make_unique<std::mutex>();
}

OutputData& OutputData::operator=(OutputData&& other)
{
	snippets = std::move(other.snippets);
	execution_time_info = std::move(other.execution_time_info);
	execution_health_info = std::move(other.execution_health_info);
	read_ppu_symbols = std::move(other.read_ppu_symbols);
	mutex = std::move(other.mutex);
	other.mutex = std::make_unique<std::mutex>();
	return *this;
}

void OutputData::merge(OutputData&& other)
{
	std::unique_lock<std::mutex> lock(*mutex);
	if (!other.snippets.empty()) {
		if (snippets.empty()) {
			for (auto& snippet : other.snippets) {
				snippets.emplace_back(std::move(snippet));
			}
		} else {
			if (snippets.size() != other.snippets.size()) {
				throw std::runtime_error(
				    "Snippet sizes need to match for InputData::merge(), but are (" +
				    std::to_string(snippets.size()) + ") vs. (" +
				    std::to_string(other.snippets.size()) + ").");
			}
			for (size_t i = 0; i < snippets.size(); ++i) {
				snippets.at(i).merge(other.snippets.at(i));
			}
		}
	}
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
	if (execution_time_info && other.execution_time_info) {
		execution_time_info->merge(*(other.execution_time_info));
	} else if (!execution_time_info) {
		execution_time_info = std::move(other.execution_time_info);
		other.execution_time_info.reset();
	}
	if (execution_health_info && other.execution_health_info) {
		execution_health_info->merge(*(other.execution_health_info));
	} else {
		execution_health_info = std::move(other.execution_health_info);
		other.execution_health_info.reset();
	}
}

void OutputData::merge(OutputData& other)
{
	merge(std::forward<OutputData>(other));
}

void OutputData::clear()
{
	std::unique_lock<std::mutex> lock(*mutex);
	for (auto& snippet : snippets) {
		snippet.clear();
	}
	execution_time_info.reset();
	execution_health_info.reset();
	read_ppu_symbols.clear();
}

bool OutputData::empty() const
{
	return std::all_of(
	           snippets.begin(), snippets.end(),
	           [](auto const& snippet) { return snippet.empty(); }) &&
	       read_ppu_symbols.empty();
}

size_t OutputData::batch_size() const
{
	size_t size = 0;
	if (!snippets.empty()) {
		size = snippets.at(0).batch_size();
		for (size_t i = 1; i < snippets.size(); ++i) {
			if (snippets.at(i).batch_size() != size) {
				throw std::runtime_error("InputData is not valid.");
			}
		}
	}
	if (!((read_ppu_symbols.size() == size) || read_ppu_symbols.empty() ||
	      std::all_of(snippets.begin(), snippets.end(), [](auto const& snippet) {
		      return snippet.empty();
	      }))) {
		throw std::runtime_error("OutputData not valid.");
	}
	return std::all_of(
	           snippets.begin(), snippets.end(),
	           [](auto const& snippet) { return snippet.empty(); })
	           ? read_ppu_symbols.size()
	           : size;
}

bool OutputData::valid() const
{
	for (auto const& snippet : snippets) {
		if (!snippet.valid()) {
			return false;
		}
	}
	if (!snippets.empty()) {
		size_t size = snippets.at(0).batch_size();
		for (size_t i = 1; i < snippets.size(); ++i) {
			if (snippets.at(i).batch_size() != size) {
				return false;
			}
		}
		return read_ppu_symbols.size() == size || read_ppu_symbols.empty();
	}
	return true;
}

} // namespace grenade::vx::signal_flow
