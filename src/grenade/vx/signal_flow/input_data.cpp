#include "grenade/vx/signal_flow/input_data.h"

#include <stdexcept>

namespace grenade::vx::signal_flow {

InputData::InputData() : snippets(), inter_batch_entry_routing_disabled(), inter_batch_entry_wait()
{
}

InputData::InputData(InputData&& other) :
    snippets(std::move(other.snippets)),
    inter_batch_entry_routing_disabled(std::move(other.inter_batch_entry_routing_disabled)),
    inter_batch_entry_wait(std::move(other.inter_batch_entry_wait))
{}

InputData& InputData::operator=(InputData&& other)
{
	snippets = std::move(other.snippets);
	inter_batch_entry_routing_disabled = std::move(other.inter_batch_entry_routing_disabled);
	inter_batch_entry_wait = std::move(other.inter_batch_entry_wait);
	return *this;
}

void InputData::merge(InputData&& other)
{
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
	if (!other.inter_batch_entry_routing_disabled.empty()) {
		inter_batch_entry_routing_disabled.merge(other.inter_batch_entry_routing_disabled);
	}
	if (!other.inter_batch_entry_wait.empty()) {
		inter_batch_entry_wait.merge(other.inter_batch_entry_wait);
	}
}

void InputData::merge(InputData& other)
{
	merge(std::forward<InputData>(other));
}

void InputData::clear()
{
	for (auto& snippet : snippets) {
		snippet.clear();
	}
	inter_batch_entry_routing_disabled.clear();
	inter_batch_entry_wait.clear();
}

bool InputData::empty() const
{
	return std::all_of(
	           snippets.begin(), snippets.end(),
	           [](auto const& snippet) { return snippet.empty(); }) &&
	       inter_batch_entry_routing_disabled.empty() && inter_batch_entry_wait.empty();
}

size_t InputData::batch_size() const
{
	if (snippets.empty()) {
		return 0;
	}
	size_t size = snippets.at(0).batch_size();
	for (size_t i = 1; i < snippets.size(); ++i) {
		if (snippets.at(i).batch_size() != size) {
			throw std::runtime_error("InputData is not valid.");
		}
	}
	return size;
}

bool InputData::valid() const
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
	}
	return true;
}

} // namespace grenade::vx::signal_flow
