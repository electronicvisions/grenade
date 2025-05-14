#include "grenade/vx/signal_flow/output_data_snippet.h"

#include <stdexcept>

namespace grenade::vx::signal_flow {

OutputDataSnippet::OutputDataSnippet() : DataSnippet(), pre_execution_chips() {}

OutputDataSnippet::OutputDataSnippet(DataSnippet&& other) :
    DataSnippet(static_cast<DataSnippet&&>(other)), pre_execution_chips()
{
}

void OutputDataSnippet::merge(OutputDataSnippet&& other)
{
	static_cast<DataSnippet&>(*this).merge(other);
	pre_execution_chips.merge(other.pre_execution_chips);
}

void OutputDataSnippet::merge(OutputDataSnippet& other)
{
	merge(std::forward<OutputDataSnippet>(other));
}

void OutputDataSnippet::clear()
{
	static_cast<DataSnippet&>(*this).clear();
	pre_execution_chips.clear();
}

bool OutputDataSnippet::empty() const
{
	return static_cast<DataSnippet const&>(*this).empty() && pre_execution_chips.empty();
}

size_t OutputDataSnippet::batch_size() const
{
	return static_cast<DataSnippet const&>(*this).batch_size();
}

bool OutputDataSnippet::valid() const
{
	return static_cast<DataSnippet const&>(*this).valid();
}

} // namespace grenade::vx::signal_flow
