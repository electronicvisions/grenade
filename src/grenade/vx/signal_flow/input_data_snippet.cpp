#include "grenade/vx/signal_flow/input_data_snippet.h"

#include <stdexcept>

namespace grenade::vx::signal_flow {

InputDataSnippet::InputDataSnippet() : DataSnippet(), runtime() {}

void InputDataSnippet::merge(InputDataSnippet&& other)
{
	static_cast<DataSnippet&>(*this).merge(other);
	if (!other.runtime.empty()) {
		if (runtime.empty()) {
			runtime = std::move(other.runtime);
		} else {
			if (runtime.size() != other.runtime.size()) {
				throw std::runtime_error(
				    "Runtime sizes need to match for InputDataSnippet::merge(), but are (" +
				    std::to_string(runtime.size()) + ") vs. (" +
				    std::to_string(other.runtime.size()) + ").");
			}
			for (size_t i = 0; i < runtime.size(); ++i) {
				runtime.at(i).merge(other.runtime.at(i));
			}
		}
	}
}

void InputDataSnippet::merge(InputDataSnippet& other)
{
	merge(std::forward<InputDataSnippet>(other));
}

void InputDataSnippet::clear()
{
	static_cast<DataSnippet&>(*this).clear();
	runtime.clear();
}

bool InputDataSnippet::empty() const
{
	return static_cast<DataSnippet const&>(*this).empty() && runtime.empty();
}

size_t InputDataSnippet::batch_size() const
{
	size_t const size = static_cast<DataSnippet const&>(*this).batch_size();
	if (!data.empty() && !runtime.empty() && size != runtime.size()) {
		throw std::runtime_error("InputDataSnippet is not valid.");
	}
	return data.empty() ? runtime.size() : size;
}

bool InputDataSnippet::valid() const
{
	size_t const size = static_cast<DataSnippet const&>(*this).batch_size();
	return data.empty() || runtime.empty() || size == runtime.size();
}

} // namespace grenade::vx::signal_flow
