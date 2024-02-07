#include "grenade/vx/signal_flow/input_data.h"

#include <stdexcept>

namespace grenade::vx::signal_flow {

InputData::InputData() : Data(), runtime() {}

InputData::InputData(InputData&& other) :
    Data(static_cast<Data&&>(other)),
    runtime(std::move(other.runtime)),
    inter_batch_entry_wait(std::move(other.inter_batch_entry_wait))
{}

InputData& InputData::operator=(InputData&& other)
{
	static_cast<Data&>(*this).operator=(static_cast<Data&&>(other));
	runtime = std::move(other.runtime);
	inter_batch_entry_wait = std::move(other.inter_batch_entry_wait);
	return *this;
}

void InputData::merge(InputData&& other)
{
	static_cast<Data&>(*this).merge(other);
	if (!other.runtime.empty()) {
		if (runtime.empty()) {
			runtime = std::move(other.runtime);
		} else {
			if (runtime.size() != other.runtime.size()) {
				throw std::runtime_error(
				    "Runtime sizes need to match for InputData::merge(), but are (" +
				    std::to_string(runtime.size()) + ") vs. (" +
				    std::to_string(other.runtime.size()) + ").");
			}
			for (size_t i = 0; i < runtime.size(); ++i) {
				runtime.at(i).merge(other.runtime.at(i));
			}
		}
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
	static_cast<Data&>(*this).clear();
	runtime.clear();
	inter_batch_entry_wait.clear();
}

bool InputData::empty() const
{
	return static_cast<Data const&>(*this).empty() && runtime.empty() &&
	       inter_batch_entry_wait.empty();
}

size_t InputData::batch_size() const
{
	size_t const size = static_cast<Data const&>(*this).batch_size();
	if (!data.empty() && !runtime.empty() && size != runtime.size()) {
		throw std::runtime_error("InputData is not valid.");
	}
	return data.empty() ? runtime.size() : size;
}

bool InputData::valid() const
{
	size_t const size = static_cast<Data const&>(*this).batch_size();
	return data.empty() || runtime.empty() || size == runtime.size();
}

} // namespace grenade::vx::signal_flow
