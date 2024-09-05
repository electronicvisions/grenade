#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"

#include "hate/indent.h"
#include "hate/join.h"

namespace grenade::vx::network::abstract {

ClockCycleTimeDomainRuntimes::ClockCycleTimeDomainRuntimes(
    std::vector<std::optional<vx::common::Time>> values,
    vx::common::Time inter_batch_entry_wait,
    bool inter_batch_entry_routing_disabled) :
    values(std::move(values)),
    inter_batch_entry_wait(inter_batch_entry_wait),
    inter_batch_entry_routing_disabled(inter_batch_entry_routing_disabled)
{
}

size_t ClockCycleTimeDomainRuntimes::batch_size() const
{
	return values.size();
}

std::unique_ptr<grenade::common::TimeDomainRuntimes> ClockCycleTimeDomainRuntimes::copy() const
{
	return std::make_unique<ClockCycleTimeDomainRuntimes>(*this);
}

std::unique_ptr<grenade::common::TimeDomainRuntimes> ClockCycleTimeDomainRuntimes::move()
{
	return std::make_unique<ClockCycleTimeDomainRuntimes>(std::move(*this));
}

bool ClockCycleTimeDomainRuntimes::is_equal_to(
    grenade::common::TimeDomainRuntimes const& other) const
{
	auto const& other_runtimes = static_cast<ClockCycleTimeDomainRuntimes const&>(other);
	return values == other_runtimes.values &&
	       inter_batch_entry_wait == other_runtimes.inter_batch_entry_wait &&
	       inter_batch_entry_routing_disabled == other_runtimes.inter_batch_entry_routing_disabled;
}

std::ostream& ClockCycleTimeDomainRuntimes::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "ClockCycleTimeDomainRuntimes(\n";
	ios << hate::Indentation("\t");
	for (auto const& v : values) {
		if (v) {
			ios << *v << "\n";
		} else {
			ios << "nullopt\n";
		}
	}
	ios << "\ninter batch entry wait: " << inter_batch_entry_wait << "\n";
	ios << "\ninter batch entry routing disabled: " << std::boolalpha
	    << inter_batch_entry_routing_disabled << hate::Indentation() << "\n)";
	return os;
}

} // namespace grenade::vx::network::abstract
