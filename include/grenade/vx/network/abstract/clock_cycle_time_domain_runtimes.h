#pragma once
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Runtimes for time domain in FPGA clock cycle units.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) ClockCycleTimeDomainRuntimes
    : public grenade::common::TimeDomainRuntimes
{
	/*
	 * Runtime for each batch entry in FPGA clock cycles.
	 **/
	std::vector<std::optional<vx::common::Time>> values;

	/**
	 * Minimal waiting time between batch entries.
	 * It is guaranteed, that the time between successive batch entries is at least the specified
	 * time. Additional time might pass due to non-realtime operations after the previous and before
	 * the next batch entry.
	 * For the case of multiple realtime columns (multiple snippets per batch
	 * entry), only the `inter_batch_entry_wait` of the last realtime column will be processed.
	 */
	vx::common::Time inter_batch_entry_wait;

	/**
	 * Whether to disable event routing between batch entries (resetting recurrent state).
	 */
	bool inter_batch_entry_routing_disabled;

	ClockCycleTimeDomainRuntimes(
	    std::vector<std::optional<vx::common::Time>> values,
	    vx::common::Time inter_batch_entry_wait,
	    bool inter_batch_entry_routing_disabled = true);

	virtual size_t batch_size() const override;

	virtual std::unique_ptr<grenade::common::TimeDomainRuntimes> copy() const override;
	virtual std::unique_ptr<grenade::common::TimeDomainRuntimes> move() override;

protected:
	virtual bool is_equal_to(grenade::common::TimeDomainRuntimes const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
