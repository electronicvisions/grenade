#include "grenade/vx/network/abstract/executor_global.h"

#include "hate/timer.h"
#include <ostream>

namespace grenade::vx::network::abstract {

std::unique_ptr<grenade::common::OutputData::Executor> ExecutorGlobal::copy() const
{
	return std::make_unique<ExecutorGlobal>(*this);
}

std::unique_ptr<grenade::common::OutputData::Executor> ExecutorGlobal::move()
{
	return std::make_unique<ExecutorGlobal>(std::move(*this));
}

bool ExecutorGlobal::is_equal_to(grenade::common::OutputData::Executor const& other) const
{
	return execution_duration == static_cast<ExecutorGlobal const&>(other).execution_duration;
}

std::ostream& ExecutorGlobal::print(std::ostream& os) const
{
	os << "ExecutorGlobal(";
	os << "execution_duration: " << hate::to_string(execution_duration) << ")";
	return os;
}

} // namespace grenade::vx::network::abstract
