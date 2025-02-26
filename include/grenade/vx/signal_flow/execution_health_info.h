#pragma once
#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/genpybind.h"
#include "halco/common/typed_heap_array.h"
#include "halco/hicann-dls/vx/v3/highspeed_link.h"
#include "halco/hicann-dls/vx/v3/routing_crossbar.h"
#include "haldls/vx/v3/arq.h"
#include "haldls/vx/v3/phy.h"
#include "haldls/vx/v3/routing_crossbar.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <map>

namespace grenade::vx {
namespace signal_flow GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * Health information of hardware setups used during execution.
 * Contains drop counter and chip-FPGA link status information.
 */
struct GENPYBIND(visible) ExecutionHealthInfo
{
	struct ExecutionInstance
	{
		haldls::vx::v3::HicannARQStatus hicann_arq_status;

		halco::common::
		    typed_heap_array<haldls::vx::v3::PhyStatus, halco::hicann_dls::vx::v3::PhyStatusOnFPGA>
		        phy_status;

		halco::common::typed_heap_array<
		    haldls::vx::CrossbarInputDropCounter,
		    halco::hicann_dls::vx::v3::CrossbarInputOnDLS>
		    crossbar_input_drop_counter;

		halco::common::typed_heap_array<
		    haldls::vx::CrossbarOutputEventCounter,
		    halco::hicann_dls::vx::v3::CrossbarOutputOnDLS>
		    crossbar_output_event_counter;

		/**
		 * Calculates difference of counter values, expects rhs > lhs.
		 */
		ExecutionInstance& operator-=(ExecutionInstance const& rhs) SYMBOL_VISIBLE;

		/**
		 * Calculates sum of counter values.
		 */
		ExecutionInstance& operator+=(ExecutionInstance const& rhs) SYMBOL_VISIBLE;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, ExecutionInstance const& data)
		    SYMBOL_VISIBLE;
	};

	std::map<common::ExecutionInstanceID, ExecutionInstance> execution_instances;

	/**
	 * Merge other execution health info.
	 * This merges all map-like structures.
	 * @param other Other execution health info to merge
	 */
	void merge(ExecutionHealthInfo& other) SYMBOL_VISIBLE;

	/**
	 * Merge other execution health info.
	 * This merges all map-like structures.
	 * @param other Other execution health info to merge
	 */
	void merge(ExecutionHealthInfo&& other) SYMBOL_VISIBLE GENPYBIND(hidden);

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, ExecutionHealthInfo const& data)
	    SYMBOL_VISIBLE;
};

} // namespace signal_flow
} // namespace grenade::vx
